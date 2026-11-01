# Cupid DSC Novatek rate-control difference

Verified on 2026-10-31 against MiCode commit
`7d10d738b2c4299f3b8549c61a01ece39f19c788` (merged Xiaomi 12 series source).
This record contains model-wide parameters only, not device-unique identities.

## Fixed upstream references

- [Panel selection](https://raw.githubusercontent.com/MiCode/Xiaomi_Kernel_OpenSource/7d10d738b2c4299f3b8549c61a01ece39f19c788/techpack/display-drivers/msm/mi_disp/mi_panel_id.h), lines 52–59:
  `is_use_nvt_dsc_config()` returns true for the public model enum `L3_42_02_0A_PANEL_ID`.
- [DSC helper and tables](https://raw.githubusercontent.com/MiCode/Xiaomi_Kernel_OpenSource/7d10d738b2c4299f3b8549c61a01ece39f19c788/techpack/display-drivers/msm/sde_dsc_helper.c):
  min row at line 44; Novatek max row at 67; non-Novatek row at 87; BPG row at 110;
  mode selection at 170; Novatek selection at 297–309.
- [PPS construction](https://raw.githubusercontent.com/MiCode/Xiaomi_Kernel_OpenSource/7d10d738b2c4299f3b8549c61a01ece39f19c788/techpack/display-drivers/msm/dsi/dsi_panel.c), lines 4435–4470.
- [PPS send ordering](https://raw.githubusercontent.com/MiCode/Xiaomi_Kernel_OpenSource/7d10d738b2c4299f3b8549c61a01ece39f19c788/techpack/display-drivers/msm/dsi/dsi_display.c), lines 8530–8547.

## Device-tree values, re-decoded from raw big-endian u32

Panel node: `qcom,mdss_dsi_l3_42_02_0a_dsc_cmd`.
All three modes use DSC 1.1, SCR revision 0, 10 bpc, 8 bpp, block prediction,
1080x12 slices, one slice per packet, 1080x2400 picture and topology (1,1,1).
Mode rates are 60/120/90 Hz. Four lanes run at 832,000,000 bits/s;
`mdp-transfer-time-us` is 7000. The uncompressed source is 30 bpp.

Older markdown values such as width=56, height=96 and transfer time=88 were
truncated to a low byte and must not be used as hardware facts.

## Complete DSC 1.1 / SCR0 / 10bpc / 8bpp Novatek ranges

| Index | Min QP | Max QP | BPG offset |
|---:|---:|---:|---:|
| 0 | 0 | 8 | 2 |
| 1 | 4 | 8 | 0 |
| 2 | 5 | 9 | 0 |
| 3 | 5 | 10 | -2 |
| 4 | 7 | 11 | -4 |
| 5 | 7 | 11 | -6 |
| 6 | 7 | 11 | -8 |
| 7 | 7 | 12 | -8 |
| 8 | 7 | 13 | -8 |
| 9 | 7 | 14 | -10 |
| 10 | 9 | 15 | -10 |
| 11 | 9 | 16 | -12 |
| 12 | 9 | 17 | -12 |
| 13 | 11 | 17 | -12 |
| 14 | 17 | 19 | -12 |

Compared to the running kernel's `drm_dsc_setup_rc_params(..., DRM_DSC_1_1_PRE_SCR)`
10bpc/8bpp table, **only range 0 max QP differs: 8 rather than 4**.
Switching to `DRM_DSC_1_2_444` would alter other ranges incorrectly.
Use the existing helper and override this single field in the Cupid panel driver.
The host's existing nonzero last-range check preserves panel-supplied RC values.

## Derived values (calculation, not a captured panel PPS)

`line_buf_depth=11`, `chunk=1080`, `rc_model_size=8192`, `initial_offset=6144`,
`initial_xmit_delay=512`, `initial_dec_delay=796`, `initial_scale_value=32`,
`first_line_bpg_offset=12`, `nfl_bpg_offset=2235`, `slice_bpg_offset=1085`,
`final_offset=4336`, `scale_increment_interval=382`, `scale_decrement_interval=15`,
`mux_word_size=48`, `extra_mux_bits=240`.

The expected standard PPS differs from PRE_SCR at zero-based byte 58 only:
`01` becomes `02`, while byte 59 stays `02`.
The reference calculated 128-byte PPS SHA-256 is
`48a09a837140a423412f01cb7cc7da9d806e0f99343db033ae79fa87b1c66f0d`.

The vendor's 19-byte private `0x91` payload contains several matching derived
values, but its full register mapping is not established. Do not treat it as a
standard 128-byte PPS or modify it on that assumption.
