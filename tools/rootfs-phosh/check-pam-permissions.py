#!/usr/bin/env python3
"""Check Debian archive modes and installed PAM helper ownership, never chmod guesses."""
import io
from pathlib import Path
import subprocess
import tarfile

ROOTFS = Path('/root/rootfs-phosh-v1')
HERE = Path(__file__).resolve().parent
# Paths only: neither passwords nor password hashes are read.
checks = {
    'libpam-modules-bin': ['usr/sbin/unix_chkpwd'],
    'passwd': ['usr/bin/passwd', 'usr/bin/chfn', 'usr/bin/chsh', 'usr/bin/gpasswd'],
    'util-linux': ['usr/bin/su'],
}
records = []
for package, paths in checks.items():
    archives = sorted((ROOTFS / 'var/cache/apt/archives').glob(package + '_*.deb'))
    if not archives:
        raise SystemExit(f'Missing original archive needed for mode comparison: {package}')
    archive = archives[-1]
    data = subprocess.run(['dpkg-deb', '--fsys-tarfile', str(archive)],
                          check=True, capture_output=True).stdout
    with tarfile.open(fileobj=io.BytesIO(data)) as handle:
        for name in paths:
            member = handle.getmember('./' + name)
            installed = (ROOTFS / name).stat()
            expected = (member.mode & 0o7777, member.uid, member.gid)
            actual = (installed.st_mode & 0o7777, installed.st_uid, installed.st_gid)
            if actual != expected:
                raise SystemExit(f'Package permission mismatch: {name}')
            records.append(f'{name}\tmode={expected[0]:04o}\tuid={expected[1]}\tgid={expected[2]}\t{archive.name}')
shadow_group = next(int(line.split(':')[2]) for line in
                    (ROOTFS / 'etc/group').read_text().splitlines()
                    if line.startswith('shadow:'))
for name in ['etc/shadow', 'etc/gshadow']:
    stat = (ROOTFS / name).stat()
    if (stat.st_mode & 0o7777, stat.st_uid, stat.st_gid) != (0o640, 0, shadow_group):
        raise SystemExit(f'Unexpected protected password file permissions: {name}')
    records.append(f'{name}\tmode=0640\tuid=0\tgid={shadow_group}')
(HERE / 'pam-permissions.txt').write_text('\n'.join(records) + '\n')
print('Archive-matched PAM/setuid modes and root:shadow password-file modes: PASS')
