#!/usr/bin/env bash
# Fetch/install only in the isolated copy, retaining the existing TUNA source.
set -Eeuo pipefail
readonly HERE=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
readonly ROOTFS=/root/rootfs-phosh-v1
[[ $(id -u) == 0 && -d $ROOTFS && ! -L $ROOTFS ]] || exit 1
cmp -s /root/rootfs-arm64/etc/apt/sources.list "$ROOTFS/etc/apt/sources.list"
[[ -x $ROOTFS/usr/sbin/policy-rc.d ]] || { printf 'Missing build service policy.\n' >&2; exit 1; }
packages=()
while IFS= read -r package; do
    [[ -n $package && $package != \#* ]] && packages+=("$package")
done < "$HERE/packages.list"
[[ ${#packages[@]} -gt 0 ]] || exit 1
bash "$HERE/in-rootfs.sh" /usr/bin/apt-get -o Acquire::Retries=3 update
bash "$HERE/in-rootfs.sh" /usr/bin/apt-get -s --no-install-recommends install "${packages[@]}" > "$HERE/install-plan.txt"
bash "$HERE/in-rootfs.sh" /usr/bin/apt-get -o Acquire::Retries=3 \
    -o Dpkg::Use-Pty=0 --yes --no-install-recommends install "${packages[@]}"
bash "$HERE/in-rootfs.sh" /usr/bin/dpkg --audit
