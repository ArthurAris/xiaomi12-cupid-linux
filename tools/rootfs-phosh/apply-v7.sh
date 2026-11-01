#!/bin/bash
# 把 overlay-v7（网络/NTP/polkit/WiFi 省电）应用到已部署的根文件系统镜像。
#
# 用法：sudo ./apply-v7.sh /path/to/rootfs.img
#
# 说明：
#   - 只修改镜像，不接触运行中的设备。
#   - 会删除 cupid-rtc-save.timer 的启用（本板 RTC 写被 SPMI 封锁）。
set -e

IMG=${1:?usage: apply-v7.sh <rootfs.img>}
HERE=$(cd -- "$(dirname -- "$0")" && pwd)
OV=$HERE/overlay-v7
MNT=$(mktemp -d /tmp/cupid-rootfs.XXXXXX)

mount -o loop,rw "$IMG" "$MNT"
trap 'umount "$MNT"; rmdir "$MNT"' EXIT

echo "== marker =="
cat "$MNT/etc/cupid-rootfs-release" 2>/dev/null || echo "missing rootfs marker" >&2
cat "$MNT/etc/hostname" 2>/dev/null || true

echo "== copy overlay =="
cp -a "$OV/." "$MNT/"
find "$MNT/usr/local/sbin" "$MNT/etc/systemd/system" "$MNT/etc/systemd/network" \
     "$MNT/etc/polkit-1" "$MNT/etc/NetworkManager" -type f \
     -exec sed -i 's/\r$//' {} + 2>/dev/null || true

chmod 755 "$MNT/usr/local/sbin/cupid-rtc-save" "$MNT/usr/local/sbin/cupid-timesync"
chmod 644 "$MNT/etc/systemd/system/cupid-rtc-save.service" \
          "$MNT/etc/systemd/system/cupid-rtc-save.timer" \
          "$MNT/etc/systemd/system/cupid-timesync.service" \
          "$MNT/etc/systemd/system/cupid-timesync.timer"

mkdir -p "$MNT/etc/systemd/system/timers.target.wants"
# RTC 写被 SPMI arbiter 封锁：保留脚本，但不调度失败的写入。
rm -f "$MNT/etc/systemd/system/timers.target.wants/cupid-rtc-save.timer"
ln -sf /etc/systemd/system/cupid-timesync.timer \
       "$MNT/etc/systemd/system/timers.target.wants/cupid-timesync.timer"
rm -f "$MNT/etc/systemd/system/multi-user.target.wants/chrony.service"

echo cupid-ubuntu-phosh-v7 > "$MNT/etc/cupid-rootfs-version"

echo "== check =="
cat "$MNT/etc/systemd/network/80-cupid-usb.network"
ls -l "$MNT/etc/systemd/system/timers.target.wants/"
cat "$MNT/etc/cupid-rootfs-version"
sync
