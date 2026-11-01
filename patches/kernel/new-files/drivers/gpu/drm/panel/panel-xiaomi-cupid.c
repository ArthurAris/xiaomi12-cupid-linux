// SPDX-License-Identifier: GPL-2.0-only
/*
 * Panel driver for the Xiaomi 12 (cupid) "l3 42 02 0a" AMOLED command-mode
 * DSI panel with DSC (1080x2400).
 *
 * Init/exit sequences are taken verbatim from the vendor device tree node
 * qcom,mdss_dsi_l3_42_02_0a_dsc_cmd via tools/gen-cupid-panel-blobs.py
 * (timing@0 = 60 Hz, timing@1 = 120 Hz, timing@2 = 90 Hz; the switch
 * commands select the rate at runtime).
 */

#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/regulator/consumer.h>

#include <drm/display/drm_dsc.h>
#include <drm/display/drm_dsc_helper.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_connector.h>
#include <drm/drm_crtc.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>
#include <drm/drm_probe_helper.h>
#include <video/mipi_display.h>

#include "panel-xiaomi-cupid-blobs.h"

#define CUPID_BL_MIN		1
#define CUPID_BL_MAX		4095
#define CUPID_BL_DEFAULT	2048

static bool full_init;
module_param(full_init, bool, 0644);
MODULE_PARM_DESC(full_init,
		 "Reset and replay the panel init sequence after the first blank (default: off)");

struct cupid_blob_set {
	unsigned int hz;
	const u8 *on;
	size_t on_len;
	const u8 *sw;
	size_t sw_len;
};

/* vendor timing@0/1/2 = 60/120/90 Hz, identical porches */
static const struct cupid_blob_set cupid_sets[] = {
	{ .hz = 60,  .on = cupid_on_60,  .on_len = sizeof(cupid_on_60),
	  .sw = cupid_sw_60,  .sw_len = sizeof(cupid_sw_60) },
	{ .hz = 90,  .on = cupid_on_90,  .on_len = sizeof(cupid_on_90),
	  .sw = cupid_sw_90,  .sw_len = sizeof(cupid_sw_90) },
	{ .hz = 120, .on = cupid_on_120, .on_len = sizeof(cupid_on_120),
	  .sw = cupid_sw_120, .sw_len = sizeof(cupid_sw_120) },
};

struct cupid_panel {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct drm_dsc_config dsc;
	struct gpio_desc *reset_gpio;
	struct regulator_bulk_data supplies[3];
	struct delayed_work bl_work;
	bool rails_on;
	bool slept;
	bool needs_init;
	bool ever_slept;
	unsigned int hz;	/* panel timing currently programmed */
	struct drm_connector *connector;
};

/*
 * Vendor command blobs carry Qualcomm's dsi_ctrl_hdr:
 *   { dtype, last, vc, ack, wait_ms, len_hi, len_lo } + payload.
 * Replay them one by one, preserving the packet type the vendor used and
 * honoring the per-command delay.
 */
static int cupid_run_blob(struct cupid_panel *ctx, const u8 *blob, size_t len,
			  bool skip_mem_write)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	size_t off = 0;
	int ret;

	while (off + 7 <= len) {
		u8 dtype = blob[off];
		u8 wait = blob[off + 4];
		u16 dlen = (blob[off + 5] << 8) | blob[off + 6];
		const u8 *payload = &blob[off + 7];
		struct mipi_dsi_msg msg = {
			.channel = dsi->channel,
			.type = dtype,
		};

		if (off + 7 + dlen > len) {
			dev_err(&dsi->dev, "truncated vendor blob\n");
			return -EINVAL;
		}
		off += 7 + dlen;

		/* vendor pads short writes with one unused byte */
		switch (dtype) {
		case MIPI_DSI_DCS_SHORT_WRITE:
			dlen = 1;
			break;
		case MIPI_DSI_DCS_SHORT_WRITE_PARAM:
			dlen = min_t(u16, dlen, 2);
			break;
		default:
			break;
		}

		/* The init sequence starts a DCS memory write for the command
		 * mode path; the video stream supplies pixels itself. */
		if (skip_mem_write && dlen &&
		    (payload[0] == MIPI_DCS_SET_COLUMN_ADDRESS ||
		     payload[0] == MIPI_DCS_SET_PAGE_ADDRESS ||
		     payload[0] == MIPI_DCS_WRITE_MEMORY_START))
			continue;

		msg.tx_len = dlen;
		msg.tx_buf = payload;
		if (dsi->mode_flags & MIPI_DSI_MODE_LPM)
			msg.flags |= MIPI_DSI_MSG_USE_LPM;

		if (!dsi->host || !dsi->host->ops || !dsi->host->ops->transfer)
			return -EOPNOTSUPP;

		ret = dsi->host->ops->transfer(dsi->host, &msg);
		if (ret < 0) {
			dev_err(&dsi->dev,
				"failed to send DSI cmd 0x%02x (type 0x%02x): %d\n",
				payload[0], dtype, ret);
			return ret;
		}

		if (wait)
			msleep(wait);
	}

	return 0;
}

/* vendor dsi_panel_pwr_supply_L3: vddio, vddd, vci, 10 ms settle each */
static const struct {
	const char *name;
	unsigned int post_on_ms;
} cupid_supplies[] = {
	{ "vddio", 10 },
	{ "vddd", 10 },
	{ "vci", 10 },
};

static inline struct cupid_panel *to_cupid_panel(struct drm_panel *panel)
{
	return container_of(panel, struct cupid_panel, panel);
}

static int cupid_power_on(struct cupid_panel *ctx)
{
	struct device *dev = &ctx->dsi->dev;
	int i, ret;

	if (ctx->rails_on)
		return 0;

	for (i = 0; i < ARRAY_SIZE(ctx->supplies); i++) {
		ret = regulator_enable(ctx->supplies[i].consumer);
		if (ret) {
			dev_err(dev, "failed to enable %s: %d\n",
				ctx->supplies[i].supply, ret);
			while (--i >= 0)
				regulator_disable(ctx->supplies[i].consumer);
			return ret;
		}
		if (cupid_supplies[i].post_on_ms)
			msleep(cupid_supplies[i].post_on_ms);
	}

	ctx->rails_on = true;
	return 0;
}

static void cupid_power_off(struct cupid_panel *ctx)
{
	if (!ctx->rails_on)
		return;

	regulator_bulk_disable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
	ctx->rails_on = false;
}

/* vendor qcom,mdss-dsi-reset-sequence = <1 11>, <0 1>, <1 11> */
static int cupid_reset_pulse(struct cupid_panel *ctx)
{
	int ret;

	ret = gpiod_direction_output(ctx->reset_gpio, 1);
	if (ret) {
		dev_err(&ctx->dsi->dev, "failed to configure reset GPIO: %d\n",
			ret);
		return ret;
	}
	usleep_range(11000, 12000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(1000, 2000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(11000, 12000);

	return 0;
}

static int cupid_prepare(struct drm_panel *panel)
{
	struct cupid_panel *ctx = to_cupid_panel(panel);
	int ret;

	dev_warn(&ctx->dsi->dev, "cupid: prepare (full_init=%d, slept=%d)\n",
		 full_init, ctx->slept);

	ret = cupid_power_on(ctx);
	if (ret)
		return ret;

	/*
	 * On the first boot the bootloader has already initialised the panel
	 * and the link cannot carry the vendor init sequence before the video
	 * stream is running (E-031).  With full_init enabled, only a panel that
	 * has been through at least one blank is reset and re-initialised by
	 * the enable work.
	 */
	if (full_init && ctx->ever_slept) {
		ret = cupid_reset_pulse(ctx);
		if (ret)
			return ret;
		ctx->needs_init = true;
	}

	return 0;
}

static int cupid_disable(struct drm_panel *panel)
{
	struct cupid_panel *ctx = to_cupid_panel(panel);
	int ret;

	dev_warn(&ctx->dsi->dev, "cupid: disable (display off + sleep in)\n");

	ctx->dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;
	ret = cupid_run_blob(ctx, cupid_off_seq, sizeof(cupid_off_seq), false);
	ctx->dsi->mode_flags |= MIPI_DSI_MODE_LPM;
	if (ret) {
		dev_warn(&ctx->dsi->dev, "sleep-in sequence failed: %d\n", ret);
		return 0;
	}

	ctx->slept = true;
	ctx->ever_slept = true;
	return 0;
}

static int cupid_unprepare(struct drm_panel *panel)
{
	struct cupid_panel *ctx = to_cupid_panel(panel);

	cancel_delayed_work_sync(&ctx->bl_work);

	if (full_init && ctx->ever_slept) {
		/* The panel can be re-initialised from scratch after this. */
		gpiod_set_value_cansleep(ctx->reset_gpio, 0);
		cupid_power_off(ctx);
		dev_warn(&ctx->dsi->dev,
			 "cupid: unprepare (full init armed, rails off)\n");
		return 0;
	}

	/*
	 * Default path: keep the rails and the panel state.  disable() has
	 * already sent the sleep-in sequence, and a full re-init is not
	 * possible before the video stream is running.
	 */
	dev_warn(&ctx->dsi->dev, "cupid: unprepare (rails kept, slept=%d)\n",
		 ctx->slept);
	return 0;
}

/*
 * vendor timing@0: H fp/bp/pw = 32/32/16, V fp/bp/pw = 64/32/16; the pixel
 * clock follows the selected refresh rate.
 */
#define CUPID_HDISPLAY	1080
#define CUPID_VDISPLAY	2400
#define CUPID_HFP	32
#define CUPID_HBP	32
#define CUPID_HPW	16
#define CUPID_VFP	64
#define CUPID_VBP	32
#define CUPID_VPW	16

static int cupid_get_modes(struct drm_panel *panel,
			   struct drm_connector *connector)
{
	struct cupid_panel *ctx = to_cupid_panel(panel);
	static const struct {
		unsigned int hz;
		unsigned int clock;
		unsigned int vtotal;
	} rates[] = {
		/*
		 * Only the validated 120 Hz timing is exposed.  The panel's
		 * vendor 90/60 Hz switch sequences do not survive a host-side
		 * vtotal change on this board: modetest at 90 Hz hung the
		 * display subsystem and rebooted the device (E-053, TD-16).
		 */
		{ 120, 349670, 2512 },
	};
	struct drm_display_mode base = {
		.hdisplay = CUPID_HDISPLAY,
		.hsync_start = CUPID_HDISPLAY + CUPID_HFP,
		.hsync_end = CUPID_HDISPLAY + CUPID_HFP + CUPID_HPW,
		.htotal = CUPID_HDISPLAY + CUPID_HFP + CUPID_HPW + CUPID_HBP,
		.vdisplay = CUPID_VDISPLAY,
		.vsync_start = CUPID_VDISPLAY + CUPID_VFP,
		.vsync_end = CUPID_VDISPLAY + CUPID_VFP + CUPID_VPW,
		.vtotal = CUPID_VDISPLAY + CUPID_VFP + CUPID_VPW + CUPID_VBP,
		.width_mm = 68,
		.height_mm = 152,
	};
	int i, count = 0;

	ctx->connector = connector;

	for (i = 0; i < ARRAY_SIZE(rates); i++) {
		struct drm_display_mode *mode;

		mode = drm_mode_duplicate(connector->dev, &base);
		if (!mode)
			return count;

		mode->clock = rates[i].clock;
		mode->vtotal = rates[i].vtotal;
		mode->type = DRM_MODE_TYPE_DRIVER;
		if (rates[i].hz == 120)
			mode->type |= DRM_MODE_TYPE_PREFERRED;
		drm_mode_set_name(mode);
		drm_mode_probed_add(connector, mode);
		count++;
	}

	return count;
}

/* Send one DCS 0x51 write; only usable while the video stream is running. */
static int cupid_apply_backlight(struct cupid_panel *ctx, u16 brightness)
{
	int ret;

	brightness = clamp_t(u16, brightness, CUPID_BL_MIN, CUPID_BL_MAX);
	ctx->dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;
	ret = mipi_dsi_dcs_set_display_brightness_large(ctx->dsi, brightness);
	ctx->dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	return ret;
}

static const struct cupid_blob_set *cupid_set_hz(unsigned int hz)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(cupid_sets); i++)
		if (cupid_sets[i].hz == hz)
			return &cupid_sets[i];
	return NULL;
}

static unsigned int cupid_target_hz(struct cupid_panel *ctx)
{
	const struct drm_display_mode *mode = NULL;

	/*
	 * The panel bridge does not pass the display mode to enable(); the
	 * connector's committed state is the only place the driver can read
	 * the timing that the DSI host was just programmed with.
	 */
	if (ctx->connector && ctx->connector->state &&
	    ctx->connector->state->crtc)
		mode = &ctx->connector->state->crtc->state->mode;

	if (mode) {
		unsigned int hz = drm_mode_vrefresh(mode);

		if (hz <= 65)
			return 60;
		if (hz <= 100)
			return 90;
	}
	return 120;
}

/*
 * Vendor refresh-switch sequences are written in the panel's 120 Hz base
 * state: normalise to 120 first, then apply the requested rate.  The video
 * stream is already running at the new timing when this is called from the
 * enable work, so only the panel timing registers need to follow.
 */
static int cupid_set_refresh(struct cupid_panel *ctx, unsigned int hz)
{
	const struct cupid_blob_set *base, *target;
	int ret;

	if (ctx->hz == hz)
		return 0;

	base = cupid_set_hz(120);
	target = cupid_set_hz(hz);
	if (!base || !target)
		return -EINVAL;

	ctx->dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	if (ctx->hz != 120) {
		ret = cupid_run_blob(ctx, base->sw, base->sw_len, false);
		if (ret) {
			dev_warn(&ctx->dsi->dev,
				 "refresh normalise to 120 Hz failed: %d\n", ret);
			goto out;
		}
		ctx->hz = 120;
	}

	ret = cupid_run_blob(ctx, target->sw, target->sw_len, false);
	if (ret) {
		dev_warn(&ctx->dsi->dev, "refresh switch to %u Hz failed: %d\n",
			 hz, ret);
		goto out;
	}

	ctx->hz = hz;
	dev_warn(&ctx->dsi->dev, "cupid: refresh switched to %u Hz\n", hz);
out:
	ctx->dsi->mode_flags |= MIPI_DSI_MODE_LPM;
	return ret;
}

static int cupid_full_init(struct cupid_panel *ctx)
{
	unsigned int hz = cupid_target_hz(ctx);
	const struct cupid_blob_set *set = cupid_set_hz(hz);
	struct drm_dsc_picture_parameter_set pps;
	int ret;

	if (!set)
		return -EINVAL;

	dev_warn(&ctx->dsi->dev, "cupid: full init start (%u Hz, %zu bytes)\n",
		 hz, set->on_len);

	ctx->dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;
	ret = cupid_run_blob(ctx, set->on, set->on_len, true);
	if (ret) {
		dev_warn(&ctx->dsi->dev, "full init sequence failed: %d\n", ret);
		goto out;
	}

	drm_dsc_pps_payload_pack(&pps, &ctx->dsc);
	ret = mipi_dsi_picture_parameter_set(ctx->dsi, &pps);
	if (ret)
		dev_warn(&ctx->dsi->dev, "full init PPS failed: %d\n", ret);
	usleep_range(1000, 2000);

	ctx->hz = hz;
	ctx->slept = false;
	dev_warn(&ctx->dsi->dev, "cupid: full init done\n");
out:
	ctx->dsi->mode_flags |= MIPI_DSI_MODE_LPM;
	return ret;
}

static int cupid_wake(struct cupid_panel *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct drm_dsc_picture_parameter_set pps;
	int ret;

	ctx->dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_exit_sleep_mode(dsi);
	if (ret) {
		dev_warn(&dsi->dev, "sleep-out failed: %d\n", ret);
		return ret;
	}
	msleep(120);

	ret = cupid_set_refresh(ctx, cupid_target_hz(ctx));
	if (ret) {
		dev_warn(&dsi->dev, "refresh switch during wake failed: %d\n", ret);
		return ret;
	}

	/* The DSC decoder loses its PPS across sleep-in; send it again before
	 * the display is turned back on.  The backlight is still off, so a
	 * short undecoded interval is not visible. */
	drm_dsc_pps_payload_pack(&pps, &ctx->dsc);
	ret = mipi_dsi_picture_parameter_set(dsi, &pps);
	if (ret)
		dev_warn(&dsi->dev, "PPS refresh failed: %d\n", ret);
	usleep_range(1000, 2000);

	ret = mipi_dsi_dcs_set_display_on(dsi);
	if (ret) {
		dev_warn(&dsi->dev, "display-on failed: %d\n", ret);
		return ret;
	}
	msleep(20);

	ctx->slept = false;
	dev_warn(&dsi->dev, "cupid: wake complete\n");
	return 0;
}

/*
 * Commands cannot be sent in prepare()/enable(): the video stream (and its
 * BLLP windows) is not up yet and the command DMA would time out.  Apply the
 * boot brightness from a delayed work once the stream has settled.
 */
static void cupid_bl_work_func(struct work_struct *work)
{
	struct cupid_panel *ctx = container_of(work, struct cupid_panel,
					       bl_work.work);
	u16 brightness;
	int ret;

	if (!ctx->panel.prepared)
		return;

	if (ctx->needs_init) {
		ret = cupid_full_init(ctx);
		if (ret)
			return;
		ctx->needs_init = false;
	} else if (ctx->slept) {
		ret = cupid_wake(ctx);
		if (ret)
			return;
	} else {
		ret = cupid_set_refresh(ctx, cupid_target_hz(ctx));
		if (ret)
			dev_warn(&ctx->dsi->dev,
				 "refresh switch on enabled panel failed: %d\n",
				 ret);
	}

	brightness = ctx->panel.backlight ?
		     backlight_get_brightness(ctx->panel.backlight) :
		     CUPID_BL_DEFAULT;
	ret = cupid_apply_backlight(ctx, brightness);
	if (ret)
		dev_warn(&ctx->dsi->dev,
			 "delayed backlight %u failed: %d\n",
			 brightness, ret);
	else
		dev_info(&ctx->dsi->dev,
			 "delayed backlight applied: %u\n",
			 brightness);
}

static int cupid_enable(struct drm_panel *panel)
{
	struct cupid_panel *ctx = to_cupid_panel(panel);

	schedule_delayed_work(&ctx->bl_work, msecs_to_jiffies(1200));

	return 0;
}

static const struct drm_panel_funcs cupid_panel_funcs = {
	.prepare = cupid_prepare,
	.enable = cupid_enable,
	.disable = cupid_disable,
	.unprepare = cupid_unprepare,
	.get_modes = cupid_get_modes,
};

static int cupid_bl_update_status(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	struct cupid_panel *ctx = mipi_dsi_get_drvdata(dsi);
	u16 brightness = backlight_get_brightness(bl);
	int ret;

	/*
	 * While the panel is off the DSI link is down; the new level stays in
	 * bl->props and is applied once the panel is powered again.
	 */
	if (!ctx->panel.prepared || ctx->slept)
		return 0;

	if (brightness && brightness < CUPID_BL_MIN)
		brightness = CUPID_BL_MIN;
	brightness = min_t(u16, brightness, CUPID_BL_MAX);

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;
	ret = mipi_dsi_dcs_set_display_brightness_large(dsi, brightness);
	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	return ret;
}

static const struct backlight_ops cupid_bl_ops = {
	.update_status = cupid_bl_update_status,
};

static struct backlight_device *cupid_create_backlight(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	const struct backlight_properties props = {
		.type = BACKLIGHT_RAW,
		.brightness = CUPID_BL_DEFAULT,
		.max_brightness = CUPID_BL_MAX,
	};

	return devm_backlight_device_register(dev, dev_name(dev), dev, dsi,
					      &cupid_bl_ops, &props);
}

static int cupid_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct cupid_panel *ctx;
	int i, ret;

	ctx = devm_drm_panel_alloc(dev, struct cupid_panel, panel,
				   &cupid_panel_funcs, DRM_MODE_CONNECTOR_DSI);
	if (IS_ERR(ctx))
		return PTR_ERR(ctx);

	for (i = 0; i < ARRAY_SIZE(ctx->supplies); i++)
		ctx->supplies[i].supply = cupid_supplies[i].name;
	ret = devm_regulator_bulk_get(dev, ARRAY_SIZE(ctx->supplies), ctx->supplies);
	if (ret < 0)
		return dev_err_probe(dev, ret, "failed to get regulators\n");

	/* the bootloader left the panel on: keep it that way until prepare */
	ctx->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_ASIS);
	if (IS_ERR(ctx->reset_gpio))
		return dev_err_probe(dev, PTR_ERR(ctx->reset_gpio),
				     "failed to get reset-gpios\n");

	ctx->dsi = dsi;
	ctx->hz = 120;	/* ABL initialises the panel at its 120 Hz timing */
	INIT_DELAYED_WORK(&ctx->bl_work, cupid_bl_work_func);
	mipi_dsi_set_drvdata(dsi, ctx);

	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	/* Native display bring-up result (E-031): video burst mode works when the
	 * panel keeps the bootloader initialization.  Sending DCS commands while
	 * the video engine runs wedges the command DMA (-110), so prepare only
	 * powers the rails and leaves the panel alone. */
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST |
			  MIPI_DSI_CLOCK_NON_CONTINUOUS;

	/* vendor DSC: 10 bits/component, 8 bits/pixel, one 1080x12 slice per line */
	ctx->dsc.dsc_version_major = 1;
	ctx->dsc.dsc_version_minor = 1;
	ctx->dsc.pic_width = CUPID_HDISPLAY;
	ctx->dsc.pic_height = CUPID_VDISPLAY;
	ctx->dsc.slice_width = CUPID_HDISPLAY;
	ctx->dsc.slice_height = 12;
	ctx->dsc.slice_count = 1;
	ctx->dsc.bits_per_component = 10;
	ctx->dsc.bits_per_pixel = 8 << 4;
	ctx->dsc.block_pred_enable = true;
	ctx->dsc.native_422 = false;
	ctx->dsc.native_420 = false;
	ret = drm_dsc_setup_rc_params(&ctx->dsc, DRM_DSC_1_1_PRE_SCR);
	if (ret)
		return dev_err_probe(dev, ret, "failed to initialize DSC rate control\n");
	/* The panel's fixed Novatek decoder uses max QP 8 in the first range. */
	ctx->dsc.rc_range_params[0].range_max_qp = 8;
	dev_info(dev, "DSC RC range0 min=%u max=%u bpg=%u\n",
		 ctx->dsc.rc_range_params[0].range_min_qp,
		 ctx->dsc.rc_range_params[0].range_max_qp,
		 ctx->dsc.rc_range_params[0].range_bpg_offset);
	dsi->dsc = &ctx->dsc;
	dsi->dsc_slice_per_pkt = 1;

	ctx->panel.prepare_prev_first = true;

	ctx->panel.backlight = cupid_create_backlight(dsi);
	if (IS_ERR(ctx->panel.backlight))
		return dev_err_probe(dev, PTR_ERR(ctx->panel.backlight),
				     "failed to create backlight\n");

	ret = devm_drm_panel_add(dev, &ctx->panel);
	if (ret)
		return ret;

	return devm_mipi_dsi_attach(dev, dsi);
}

static const struct of_device_id cupid_of_match[] = {
	{ .compatible = "xiaomi,cupid-l3-42-02-0a" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, cupid_of_match);

static struct mipi_dsi_driver cupid_driver = {
	.probe = cupid_probe,
	.driver = {
		.name = "panel-xiaomi-cupid",
		.of_match_table = cupid_of_match,
	},
};
module_mipi_dsi_driver(cupid_driver);

MODULE_DESCRIPTION("Panel driver for the Xiaomi 12 (cupid) AMOLED DSC command mode panel");
MODULE_LICENSE("GPL");
