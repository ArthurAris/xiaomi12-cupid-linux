#!/usr/bin/env bash
# Create a permission-preserving offline copy; never enter or modify the base.
set -Eeuo pipefail
readonly HERE=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
readonly BASE=/root/rootfs-arm64
readonly ROOTFS=/root/rootfs-phosh-v1
[[ $(id -u) == 0 ]] || { printf 'Run as WSL root.\n' >&2; exit 1; }
[[ -d $BASE && ! -L $BASE ]] || exit 1
[[ ! -e $ROOTFS && ! -L $ROOTFS ]] || { printf 'Refusing to overwrite %s\n' "$ROOTFS" >&2; exit 1; }
python3 - "$BASE" <<'PY'
import pathlib, sys
base = sys.argv[1]
for line in pathlib.Path('/proc/self/mountinfo').read_text().splitlines():
    target = line.split()[4].replace('\\040', ' ')
    if target == base or target.startswith(base + '/'):
        raise SystemExit('Refusing to copy a rootfs with live mounts')
PY
# Content hashes are only for generic OS configuration and the package database.
# No device data, credentials, private directory, or kernel is inspected.
(
    cd "$BASE"
    sha256sum etc/fstab etc/apt/sources.list var/lib/dpkg/status \
        etc/systemd/system/serial-getty@ttyGS0.service.d/autologin.conf
) > "$HERE/base-protected.sha256"
cp -a --reflink=auto -- "$BASE" "$ROOTFS"
# Service starts are denied before any package maintainer script is run.
install -o root -g root -m 0755 "$HERE/overlay/usr/sbin/policy-rc.d" "$ROOTFS/usr/sbin/policy-rc.d"
printf 'Created offline copy at %s\n' "$ROOTFS"
