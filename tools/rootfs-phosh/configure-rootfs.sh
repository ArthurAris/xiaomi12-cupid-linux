#!/usr/bin/env bash
# Offline configuration only. No host systemctl/loginctl, device, or kernel access.
set -Eeuo pipefail
readonly HERE=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
readonly ROOTFS=/root/rootfs-phosh-v1
[[ $(id -u) == 0 && -d $ROOTFS && ! -L $ROOTFS ]] || exit 1
inside() { bash "$HERE/in-rootfs.sh" "$@"; }

# Verify the real packaged development unit rather than installing a guessed one.
python3 - "$ROOTFS" "$HERE" <<'PY'
from pathlib import Path
import sys
root, here = map(Path, sys.argv[1:])
unit = root / 'usr/lib/systemd/system/phosh.service'
text = unit.read_text()
for required in ('ExecStart=capsh --noamb -- -c "exec /usr/bin/phosh-session"',
                 'PAMName=login', 'TTYPath=/dev/tty7', 'ExecStartPost=+chvt 7'):
    if required not in text:
        raise SystemExit('Packaged phosh.service changed; review its startup/PAM/TTY integration.')
(here / 'packaged-phosh.service').write_text(text)
session = root / 'usr/share/gnome-session/sessions/phosh.session'
if 'org.gnome.SettingsDaemon.Keyboard' not in session.read_text():
    raise SystemExit('Phosh session definition changed; review minimal dependencies.')
PY
python3 "$HERE/validate-schema.py"
python3 "$HERE/check-pam-permissions.py"

# Install only our small overlay; chmod applies to these regular config files,
# never to distribution PAM helpers or existing installed executables.
while IFS= read -r -d '' source; do
    relative=${source#"$HERE/overlay/"}
    mode=0644
    [[ $relative == usr/sbin/policy-rc.d ]] && mode=0755
    install -D -o root -g root -m "$mode" "$source" "$ROOTFS/$relative"
done < <(find "$HERE/overlay" -type f -print0)

inside /bin/bash -se <<'CHROOT'
for group in video render input audio; do
    getent group "$group" >/dev/null || groupadd --system "$group"
done
if getent passwd linux >/dev/null; then
    [[ $(id -u linux) == 1000 && $(getent passwd linux | cut -d: -f6) == /home/linux ]]
    usermod -a -G video,render,input,audio linux
else
    ! getent passwd 1000 >/dev/null
    useradd --create-home --user-group --uid 1000 --shell /bin/bash \
        --groups video,render,input,audio linux
fi
install -d -o linux -g linux -m 0700 /home/linux/.config
# Lock password-based root authentication without expiring root's account.
# agetty --autologin root bypasses the auth phase, not account/session PAM.
usermod --lock root
usermod --expiredate '' root
install -d -o root -g root -m 0755 /var/lib/systemd/linger
install -o root -g root -m 0644 /dev/null /var/lib/systemd/linger/linux
update-alternatives --set Phosh-OSK /usr/share/applications/sm.puri.Squeekboard.desktop
# Ubuntu squeekboard 1.43.1-2 points the service alternative at a DIRECTORY.
# Keep package files/alternatives intact, override only systemd's search path.
if [[ -d /usr/lib/systemd/user/mobi.phosh.OSK.service ]]; then
    source=/usr/lib/systemd/user/mobi.phosh.OSK.service/mobi.phosh.OSK.service
    [[ -f $source ]]
    install -d -m 0755 /etc/systemd/user
    ln -sfn /usr/share/squeekboard/sm.puri.Squeekboard.service/mobi.phosh.OSK.service \
        /etc/systemd/user/mobi.phosh.OSK.service
fi
glib-compile-schemas --strict /usr/share/glib-2.0/schemas
# --root=/ guarantees these are filesystem-only operations, even in chroot.
systemctl --root=/ enable phosh.service
systemctl --root=/ set-default graphical.target
systemctl --root=/ disable getty@tty1.service
# The only debug channel remains the inherited serial-getty. Prevent either
# SSH socket activation or starting a competing seatd alongside logind.
systemctl --root=/ disable ssh.socket ssh.service seatd.service
systemctl --root=/ mask ssh.socket ssh.service sshd.service sshd@.service seatd.service
CHROOT

# PIN is generated once on the host and is never included in this script/image.
python3 "$HERE/set-local-pin.py"

# Root selection/mount policy belongs to init v18, not this desktop overlay.
cmp /root/rootfs-arm64/etc/fstab "$ROOTFS/etc/fstab"
cmp /root/rootfs-arm64/etc/systemd/system/serial-getty@ttyGS0.service.d/autologin.conf \
    "$ROOTFS/etc/systemd/system/serial-getty@ttyGS0.service.d/autologin.conf"
inside /bin/sh -c 'test -f /etc/systemd/user/mobi.phosh.OSK.service; test -x /usr/bin/squeekboard; test -x /usr/bin/gnome-session; id linux; passwd -S root; passwd -S linux'
printf 'Configured %s; build policy still active until final offline validation/cleanup.\n' "$ROOTFS"
