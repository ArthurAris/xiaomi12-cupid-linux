#!/bin/sh
uname -a
cat /proc/uptime
echo "== root =="
findmnt / | head -2
df -h / | tail -1
cat /etc/cupid-rootfs-release
echo "== desktop =="
systemctl is-active phosh
cat /sys/class/drm/card0-DSI-1/status
echo "== boot image path =="
ls -l /boot/Image 2>/dev/null | head -2
echo "== battery =="
for f in capacity status temp; do printf '%s=' "$f"; cat "/sys/class/power_supply/qcom-battmgr-bat/$f" 2>/dev/null; done
