#!/usr/bin/env bash
# Run only in the offline copy: no host /dev, /sys, /run, or systemd sockets.
set -Eeuo pipefail
readonly ROOTFS=/root/rootfs-phosh-v1
if [[ ${1:-} != --mount-namespace ]]; then
    [[ $(id -u) == 0 ]] || { printf 'Run as WSL root.\n' >&2; exit 1; }
    [[ -d $ROOTFS && ! -L $ROOTFS ]] || { printf 'Missing offline copy.\n' >&2; exit 1; }
    [[ $(readlink -f "$ROOTFS") == "$ROOTFS" ]] || exit 1
    exec unshare --mount --pid --fork --kill-child -- bash "$0" --mount-namespace "$@"
fi
shift
[[ $# -gt 0 ]] || { printf 'A chroot command is required.\n' >&2; exit 2; }
mount --make-rprivate /
# All mounts are namespace-local and vanish when this invocation exits.
mount -t proc -o nosuid,nodev,noexec proc "$ROOTFS/proc"
mount -t tmpfs -o nosuid,nodev,mode=755 tmpfs "$ROOTFS/run"
mount -t tmpfs -o nosuid,nodev,mode=1777 tmpfs "$ROOTFS/tmp"
mount -t tmpfs -o nosuid,mode=755 tmpfs "$ROOTFS/dev"
mount -t tmpfs -o nosuid,nodev,noexec,mode=555 tmpfs "$ROOTFS/sys"
for spec in 'null 1 3 666' 'zero 1 5 666' 'full 1 7 666' 'random 1 8 666' 'urandom 1 9 666' 'tty 5 0 666'; do
    read -r name major minor mode <<< "$spec"
    mknod -m "$mode" "$ROOTFS/dev/$name" c "$major" "$minor"
done
mkdir -p "$ROOTFS/dev/pts" "$ROOTFS/dev/shm" "$ROOTFS/run/lock"
mount -t devpts -o newinstance,nosuid,noexec,ptmxmode=0666,mode=0620 devpts "$ROOTFS/dev/pts"
mount -t tmpfs -o nosuid,nodev,mode=1777 tmpfs "$ROOTFS/dev/shm"
ln -s pts/ptmx "$ROOTFS/dev/ptmx"
ln -s /proc/self/fd "$ROOTFS/dev/fd"
ln -s /proc/self/fd/0 "$ROOTFS/dev/stdin"
ln -s /proc/self/fd/1 "$ROOTFS/dev/stdout"
ln -s /proc/self/fd/2 "$ROOTFS/dev/stderr"
# env -i prevents the WSL host session's D-Bus/display/credentials leaking in.
exec chroot "$ROOTFS" /usr/bin/env -i \
    HOME=/root PATH=/usr/sbin:/usr/bin:/sbin:/bin \
    LANG=C.UTF-8 LC_ALL=C.UTF-8 TERM=dumb \
    DEBIAN_FRONTEND=noninteractive container=chroot \
    "$@"
