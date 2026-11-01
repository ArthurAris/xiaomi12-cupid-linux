// SPDX-License-Identifier: GPL-2.0-only
/*
 * v21: v20 + prefer the full-size ext4 root on the userdata partition
 * (label "linux"); fall back to the 2 GiB cust test root.
 * v20: v18 root verification and 240-second bootstrap guard, with one isolated
 * configfs RNDIS + ACM attempt. Boot with g_serial.enable=0. Never steal an
 * already active legacy gadget. On failure release every owned function before
 * enabling legacy ACM, so the rescue shell and Ubuntu getty still use ttyGS0.
 * No IP addressing, routes, credentials, host sharing, or storage changes here.
 */
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/prctl.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <sys/wait.h>
#include <unistd.h>

#define ROOT_MARKER "cupid-ubuntu-phosh-v1\n"
#define ROOT_SECTORS "4194304\n"

#define USB_GADGET "/sys/kernel/config/usb_gadget/cupid_v20"
#define USB_CONFIG USB_GADGET "/configs/c.1"
#define USB_RNDIS USB_GADGET "/functions/rndis.usb0"
#define USB_ACM USB_GADGET "/functions/acm.usb0"
#define GS_ENABLE "/sys/module/g_serial/parameters/enable"
/* fs/configfs/mount.c; not exported by the cross sysroot's linux/magic.h. */
#define USB_CONFIGFS_MAGIC 0x62656570L

static bool usb_gadget_owned;
static bool usb_serial_ready;
static bool usb_fallback_attempted;

static void log_message(const char *message)
{
	int fd = open("/dev/kmsg", O_WRONLY | O_CLOEXEC);

	if (fd >= 0) {
		dprintf(fd, "<6>cupid20: %s\n", message);
		close(fd);
	}
}

static ssize_t read_file(const char *path, char *buf, size_t size)
{
	int fd = open(path, O_RDONLY | O_CLOEXEC);
	ssize_t count;

	if (fd < 0)
		return -1;
	count = read(fd, buf, size - 1);
	close(fd);
	if (count >= 0)
		buf[count] = '\0';
	return count;
}

static bool has_line(const char *buf, const char *line)
{
	size_t len = strlen(line);

	while (*buf) {
		if (!strncmp(buf, line, len) && (buf[len] == '\n' || !buf[len]))
			return true;
		buf = strchr(buf, '\n');
		if (!buf)
			break;
		buf++;
	}
	return false;
}

static bool target_board(void)
{
	char compatible[256];
	ssize_t len = read_file("/proc/device-tree/compatible", compatible,
				 sizeof(compatible));
	size_t pos;

	if (len <= 0)
		return false;
	for (pos = 0; pos < (size_t)len; pos += strlen(compatible + pos) + 1) {
		if (!strcmp(compatible + pos, "xiaomi,cupid"))
			return true;
	}
	return false;
}

static bool valid_superblock(const char *device)
{
	unsigned char sb[1024];
	int fd = open(device, O_RDONLY | O_CLOEXEC);
	ssize_t count;

	if (fd < 0)
		return false;
	count = pread(fd, sb, sizeof(sb), 1024);
	close(fd);
	return count == sizeof(sb) && sb[0x38] == 0x53 && sb[0x39] == 0xef &&
	       !memcmp(sb + 0x78, "linux\0", 6);
}

/*
 * Scan for a partition by GPT name. When @exact_sectors is non-NULL the size
 * must match exactly, otherwise any size is accepted. The ext4 label check in
 * valid_superblock() makes a stray f2fs userdata harmless.
 */
static bool find_named_root(char *device, size_t size, const char *partname,
			    const char *exact_sectors)
{
	DIR *dir = opendir("/sys/class/block");
	struct dirent *entry;
	char path[512], buf[2048], wanted[64];
	unsigned int matches = 0;

	if (!dir)
		return false;
	snprintf(wanted, sizeof(wanted), "PARTNAME=%s", partname);
	while ((entry = readdir(dir))) {
		if (entry->d_name[0] == '.')
			continue;
		snprintf(path, sizeof(path), "/sys/class/block/%s/uevent", entry->d_name);
		if (read_file(path, buf, sizeof(buf)) <= 0 || !has_line(buf, wanted))
			continue;
		if (exact_sectors) {
			snprintf(path, sizeof(path), "/sys/class/block/%s/size", entry->d_name);
			if (read_file(path, buf, sizeof(buf)) <= 0 ||
			    strcmp(buf, exact_sectors)) {
				matches = 2;
				break;
			}
		}
		if (matches++ || snprintf(device, size, "/dev/%s", entry->d_name) >= (int)size) {
			matches = 2;
			break;
		}
	}
	closedir(dir);
	return matches == 1 && valid_superblock(device);
}

/* Full-size root first, then the old 2 GiB test partition. */
static bool find_root(char *device, size_t size)
{
	if (find_named_root(device, size, "userdata", NULL))
		return true;
	return find_named_root(device, size, "cust", ROOT_SECTORS);
}

static bool marker_matches(void)
{
	char buf[128];

	return read_file("/newroot/etc/cupid-rootfs-release", buf, sizeof(buf)) > 0 &&
	       !strcmp(buf, ROOT_MARKER) && access("/newroot/sbin/init", X_OK) == 0;
}

static int usb_result(const char *operation, const char *path, int rc)
{
	char message[512];

	snprintf(message, sizeof(message), "USB %s %s rc=%d%s%s", operation,
		 path, rc, rc ? " error=" : "", rc ? strerror(-rc) : "");
	log_message(message);
	return rc;
}

static int usb_write(const char *path, const char *value)
{
	size_t len = strlen(value);
	int fd = open(path, O_WRONLY | O_CLOEXEC);
	int rc;
	ssize_t count;

	if (fd < 0)
		return usb_result("open for write", path, -errno);
	/* A sysfs/configfs attribute is one transaction, never append a suffix. */
	do {
		count = write(fd, value, len);
	} while (count < 0 && errno == EINTR);
	rc = count < 0 ? -errno : ((size_t)count == len ? 0 : -EIO);
	if (close(fd) && !rc)
		rc = -errno;
	return usb_result("write", path, rc);
}

static int usb_mkdir(const char *path)
{
	int rc = mkdir(path, 0755) ? -errno : 0;

	/* EEXIST is not success: never take ownership of another gadget. */
	return usb_result("mkdir", path, rc);
}

static int usb_link(const char *target, const char *path)
{
	int rc = symlink(target, path) ? -errno : 0;

	return usb_result("link", path, rc);
}

static int usb_legacy_enabled(void)
{
	char value[8];

	if (read_file(GS_ENABLE, value, sizeof(value)) <= 0)
		return usb_result("read legacy enable", GS_ENABLE, -EIO);
	if (!strcmp(value, "Y\n") || !strcmp(value, "1\n"))
		return 1;
	if (!strcmp(value, "N\n") || !strcmp(value, "0\n"))
		return 0;
	return usb_result("invalid legacy enable", GS_ENABLE, -EINVAL);
}

static int usb_unique_udc(char *name, size_t size)
{
	DIR *dir = opendir("/sys/class/udc");
	struct dirent *entry;
	unsigned int found = 0;
	int rc = 0;

	if (!dir)
		return -errno;
	for (;;) {
		errno = 0;
		entry = readdir(dir);
		if (!entry) {
			if (errno)
				rc = -errno;
			break;
		}
		if (entry->d_name[0] == '.')
			continue;
		if (++found != 1) {
			rc = -EBUSY;
			break;
		}
		if (snprintf(name, size, "%s", entry->d_name) >= (int)size) {
			rc = -ENAMETOOLONG;
			break;
		}
	}
	if (closedir(dir) && !rc)
		rc = -errno;
	return rc ? rc : (found == 1 ? 0 : -ENODEV);
}

static bool usb_wait_serial(void)
{
	unsigned int tries;

	for (tries = 0; tries < 10; tries++) {
		if (!access("/dev/ttyGS0", F_OK) &&
		    !access("/sys/class/tty/ttyGS0", F_OK))
			return true;
		sleep(1);
	}
	log_message("USB ttyGS0 did not appear; retaining bootstrap watchdog");
	return false;
}

static bool usb_cleanup(void)
{
	static const struct {
		const char *path;
		bool link;
	} owned_paths[] = {
		{ USB_GADGET "/os_desc/c.1", true },
		{ USB_CONFIG "/acm.usb0", true },
		{ USB_CONFIG "/rndis.usb0", true },
		{ USB_ACM, false },
		{ USB_RNDIS, false },
		{ USB_CONFIG "/strings/0x409", false },
		{ USB_CONFIG, false },
		{ USB_GADGET "/strings/0x409", false },
		{ USB_GADGET, false },
	};
	char bound[128];
	bool clean = true;
	size_t i;

	if (!usb_gadget_owned)
		return true;
	if (read_file(USB_GADGET "/UDC", bound, sizeof(bound)) < 0) {
		usb_result("read before cleanup", USB_GADGET "/UDC", -EIO);
		return false;
	}
	bound[strcspn(bound, "\n")] = '\0';
	/* A failed bind normally leaves UDC empty. Unbind only if registered. */
	if (bound[0] && usb_write(USB_GADGET "/UDC", "\n"))
		return false;
	for (i = 0; i < sizeof(owned_paths) / sizeof(owned_paths[0]); i++) {
		int ret = owned_paths[i].link ? unlink(owned_paths[i].path) :
					       rmdir(owned_paths[i].path);
		int rc = ret && errno != ENOENT ? -errno : 0;

		usb_result(owned_paths[i].link ? "unlink" : "rmdir",
			   owned_paths[i].path, rc);
		if (rc)
			clean = false;
	}
	if (clean) {
		usb_gadget_owned = false;
		log_message("USB owned gadget removed; ACM function instance released");
	}
	return clean;
}

static bool usb_fallback(void)
{
	int enabled;

	if (usb_fallback_attempted)
		return usb_serial_ready;
	usb_fallback_attempted = true;
	log_message("USB attempting legacy ACM fallback");
	if (!usb_cleanup()) {
		log_message("USB cleanup incomplete; refusing a second gadget on dirty state");
		return false;
	}
	enabled = usb_legacy_enabled();
	if (enabled < 0)
		return false;
	if (!enabled) {
		/* A surviving ACM instance would force g_serial onto ttyGS1. */
		if (!access("/sys/class/tty/ttyGS0", F_OK) || errno != ENOENT) {
			log_message("USB ttyGS0 still allocated; refusing wrong-port legacy fallback");
			return false;
		}
		if (usb_write(GS_ENABLE, "1\n"))
			return false;
	}
	usb_serial_ready = usb_wait_serial();
	if (usb_serial_ready)
		log_message("USB legacy ACM ttyGS0 ready; host enumeration unverified");
	return usb_serial_ready;
}

static bool setup_usb(void)
{
	static const char *const directories[] = {
		USB_GADGET "/strings/0x409",
		/* The configs and functions parent groups are created by configfs. */
		USB_CONFIG,
		USB_CONFIG "/strings/0x409",
		USB_RNDIS,
		USB_ACM,
	};
	static const struct {
		const char *path;
		const char *value;
	} attributes[] = {
		/* Linux multifunction development IDs, not a production VID/PID. */
		{ USB_GADGET "/idVendor", "0x1d6b\n" },
		{ USB_GADGET "/idProduct", "0x0104\n" },
		{ USB_GADGET "/bcdDevice", "0x0020\n" },
		{ USB_GADGET "/bcdUSB", "0x0200\n" },
		/* IAD composite parent: usbccgp can split RNDIS and ACM on Windows. */
		{ USB_GADGET "/bDeviceClass", "0xef\n" },
		{ USB_GADGET "/bDeviceSubClass", "0x02\n" },
		{ USB_GADGET "/bDeviceProtocol", "0x01\n" },
		{ USB_GADGET "/strings/0x409/serialnumber", "cupid-mainline\n" },
		{ USB_GADGET "/strings/0x409/manufacturer", "Linux\n" },
		{ USB_GADGET "/strings/0x409/product", "Cupid RNDIS + ACM\n" },
		{ USB_CONFIG "/strings/0x409/configuration", "RNDIS + ACM\n" },
		/* Match the self-powered legacy gadget; do not change charging policy. */
		{ USB_CONFIG "/bmAttributes", "0xc0\n" },
		{ USB_CONFIG "/MaxPower", "2\n" },
		/* Fixed test-only locally administered addresses; no unit identity. */
		{ USB_RNDIS "/host_addr", "02:00:00:00:42:01\n" },
		{ USB_RNDIS "/dev_addr", "02:00:00:00:42:02\n" },
		/* This kernel requires exactly one %d, not the literal usb0. */
		{ USB_RNDIS "/ifname", "usb%d\n" },
		{ USB_RNDIS "/class", "0xe0\n" },
		{ USB_RNDIS "/subclass", "0x01\n" },
		{ USB_RNDIS "/protocol", "0x03\n" },
		{ USB_ACM "/protocol", "0x01\n" },
		{ USB_GADGET "/os_desc/use", "1\n" },
		{ USB_GADGET "/os_desc/b_vendor_code", "0xcd\n" },
		{ USB_GADGET "/os_desc/qw_sign", "MSFT100\n" },
		{ USB_RNDIS "/os_desc/interface.rndis/compatible_id", "RNDIS\n" },
		{ USB_RNDIS "/os_desc/interface.rndis/sub_compatible_id", "5162001\n" },
	};
	char udc[128], path[256], value[128];
	struct statfs fs;
	int rc, tries;
	size_t i;

	log_message("USB v20 one-shot RNDIS + ACM setup");
	rc = usb_legacy_enabled();
	if (rc) {
		log_message("USB legacy not disabled or unavailable; skipping configfs attempt");
		return usb_fallback();
	}
	for (tries = 0; tries < 15; tries++) {
		rc = usb_unique_udc(udc, sizeof(udc));
		if (rc != -ENOENT && rc != -ENODEV)
			break;
		sleep(1);
	}
	if (rc) {
		usb_result("find unique UDC", "/sys/class/udc", rc);
		goto fallback;
	}
	snprintf(path, sizeof(path), "/sys/class/udc/%s/function", udc);
	if (read_file(path, value, sizeof(value)) < 0 || value[0]) {
		usb_result("UDC must be unused", path, -EBUSY);
		goto fallback;
	}
	if (mkdir("/sys/kernel/config", 0755) && errno != EEXIST) {
		usb_result("mkdir", "/sys/kernel/config", -errno);
		goto fallback;
	}
	if (mount("configfs", "/sys/kernel/config", "configfs",
		  MS_NOSUID | MS_NODEV | MS_NOEXEC, NULL) && errno != EBUSY) {
		usb_result("mount configfs", "/sys/kernel/config", -errno);
		goto fallback;
	}
	if (statfs("/sys/kernel/config", &fs) || fs.f_type != USB_CONFIGFS_MAGIC) {
		usb_result("verify configfs mount", "/sys/kernel/config", -EINVAL);
		goto fallback;
	}
	if (usb_mkdir(USB_GADGET))
		goto fallback;
	usb_gadget_owned = true;
	for (i = 0; i < sizeof(directories) / sizeof(directories[0]); i++) {
		if (usb_mkdir(directories[i]))
			goto fallback;
	}
	/* Allocation happens at mkdir(acm.*), not at UDC bind. */
	if (read_file(USB_ACM "/port_num", value, sizeof(value)) <= 0 ||
	    strcmp(value, "0\n")) {
		usb_result("ACM must own ttyGS0", USB_ACM "/port_num", -EBUSY);
		goto fallback;
	}
	for (i = 0; i < sizeof(attributes) / sizeof(attributes[0]); i++) {
		if (usb_write(attributes[i].path, attributes[i].value))
			goto fallback;
	}
	/* Link order is interface order: RNDIS 0/1, ACM 2/3, one configuration. */
	if (usb_link(USB_RNDIS, USB_CONFIG "/rndis.usb0") ||
	    usb_link(USB_ACM, USB_CONFIG "/acm.usb0") ||
	    usb_link(USB_CONFIG, USB_GADGET "/os_desc/c.1"))
		goto fallback;
	/* Exactly one bind attempt. Never retry a failed bind against this UDC. */
	if (usb_write(USB_GADGET "/UDC", udc))
		goto fallback;
	if (read_file(USB_GADGET "/UDC", value, sizeof(value)) <= 0 ||
	    !has_line(value, udc) || !usb_wait_serial() ||
	    access("/sys/class/net/usb0", F_OK)) {
		usb_result("verify bound ACM and usb0", USB_GADGET, -ENODEV);
		goto fallback;
	}
	usb_serial_ready = true;
	log_message("USB composite bound once: ttyGS0 + usb0; host enumeration unverified");
	return true;

fallback:
	return usb_fallback();
}

static void rescue(void)
{
	if (!usb_serial_ready)
		(void)usb_fallback();
	for (;;) {
		pid_t child;

		if (access("/dev/ttyGS0", F_OK)) {
			sleep(1);
			continue;
		}
		child = fork();
		if (child == 0) {
			int fd;

			setsid();
			fd = open("/dev/ttyGS0", O_RDWR);
			if (fd >= 0) {
				dup2(fd, STDIN_FILENO);
				dup2(fd, STDOUT_FILENO);
				dup2(fd, STDERR_FILENO);
				if (fd > STDERR_FILENO)
					close(fd);
			}
			execl("/bin/sh", "sh", "-i", NULL);
			_exit(127);
		}
		if (child > 0)
			waitpid(child, NULL, 0);
		sleep(1);
	}
}

static void fail(const char *message)
{
	log_message(message);
	rescue();
}

static void make_dir(const char *path)
{
	if (mkdir(path, 0755) && errno != EEXIST)
		fail("mkdir failed");
}

static void watchdog(int runfd)
{
	unsigned int seconds;

	prctl(PR_SET_NAME, "cupid_boot_guard", 0, 0, 0);
	/* The directory descriptor follows /run across MS_MOVE and chroot. */
	for (seconds = 0; seconds < 240; seconds++) {
		int fd = openat(runfd, "cupid-boot-ok", O_RDONLY | O_NOFOLLOW | O_NONBLOCK);

		if (fd >= 0) {
			struct stat st;
			char value[16] = {0};
			bool ready = false;

			if (!fstat(fd, &st) && S_ISREG(st.st_mode) && st.st_uid == 0) {
				ssize_t n = read(fd, value, sizeof(value) - 1);

				ready = n == 6 && !strcmp(value, "ready\n");
			}

			close(fd);
			if (ready) {
				log_message("userspace accepted watchdog handoff");
				_exit(0);
			}
		}
		sleep(1);
	}
	log_message("bootstrap deadline expired, returning to Android");
	/* A blocked filesystem sync must not hold the bootstrap guard forever. */
	{
		pid_t syncer = fork();

		if (syncer == 0) {
			sync();
			_exit(0);
		}
		if (syncer > 0) {
			unsigned int wait;

			for (wait = 0; wait < 5; wait++) {
				if (waitpid(syncer, NULL, WNOHANG) == syncer)
					break;
				sleep(1);
			}
			if (wait == 5)
				kill(syncer, SIGKILL);
		}
	}
	reboot(RB_AUTOBOOT);
	_exit(1);
}

int main(void)
{
	char device[128] = "";
	const char *const mounts[] = {"dev", "proc", "sys", "run"};
	char source[32], target[64];
	int runfd, fd, tries;
	pid_t child;
	size_t i;

	setenv("PATH", "/usr/sbin:/usr/bin:/sbin:/bin", 1);
	make_dir("/dev");
	if (mount("devtmpfs", "/dev", "devtmpfs", 0, NULL) && errno != EBUSY)
		fail("devtmpfs mount failed");
	make_dir("/run");
	if (mount("tmpfs", "/run", "tmpfs", MS_NOSUID | MS_NODEV, "mode=0755"))
		fail("run mount failed");
	runfd = open("/run", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
	if (runfd < 0)
		fail("cannot open watchdog directory");
	child = fork();
	if (child == 0)
		watchdog(runfd);
	close(runfd);
	if (child < 0)
		fail("cannot start bootstrap watchdog");

	make_dir("/proc");
	make_dir("/sys");
	make_dir("/dev/pts");
	make_dir("/newroot");
	if (mount("proc", "/proc", "proc", 0, NULL) ||
	    mount("sysfs", "/sys", "sysfs", 0, NULL) ||
	    mount("devpts", "/dev/pts", "devpts", 0, "mode=0620,ptmxmode=0666"))
		fail("API filesystem mount failed");
	fd = open("/proc/sys/kernel/printk", O_WRONLY | O_CLOEXEC);
	if (fd >= 0) {
		if (write(fd, "1 4 1 7", 7) < 0)
			log_message("cannot lower console log level");
		close(fd);
	}
	if (!target_board())
		fail("board identity mismatch");
	if (!setup_usb())
		fail("USB setup and safe serial fallback unavailable");
	for (tries = 0; tries < 30; tries++) {
		if (find_root(device, sizeof(device)))
			break;
		sleep(1);
	}
	if (tries == 30)
		fail("no verified Linux root partition found");
	/* Do not replay a journal until the rootfs identity has been checked. */
	if (mount(device, "/newroot", "ext4", MS_RDONLY, "noload"))
		fail("read-only root probe failed");
	if (!marker_matches()) {
		umount("/newroot");
		fail("rootfs identity mismatch");
	}
	if (umount("/newroot") || mount(device, "/newroot", "ext4", 0, NULL))
		fail("verified root mount failed");
	if (!marker_matches())
		fail("rootfs identity changed after journal recovery");
	fd = open("/run/cupid-root-device", O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
	if (fd < 0)
		fail("cannot record root identity");
	dprintf(fd, "%s\n", device);
	close(fd);
	log_message("verified Linux root; switching to Ubuntu");

	for (i = 0; i < sizeof(mounts) / sizeof(mounts[0]); i++) {
		snprintf(source, sizeof(source), "/%s", mounts[i]);
		snprintf(target, sizeof(target), "/newroot/%s", mounts[i]);
		make_dir(target);
		if (mount(source, target, NULL, MS_MOVE, NULL))
			fail("API mount move failed");
	}
	if (chdir("/newroot") || mount(".", "/", NULL, MS_MOVE, NULL) ||
	    chroot(".") || chdir("/"))
		fail("switch_root failed");
	fd = open("/dev/console", O_RDWR);
	if (fd >= 0) {
		dup2(fd, STDIN_FILENO);
		dup2(fd, STDOUT_FILENO);
		dup2(fd, STDERR_FILENO);
		if (fd > STDERR_FILENO)
			close(fd);
	}
	execl("/sbin/init", "/sbin/init", NULL);
	fail("systemd exec failed");
	return 1;
}
