#!/bin/sh
set -eu
socket=$(find /run/user/1000 -maxdepth 1 -type s -name 'wayland-*' | sort | head -1)
[ -n "$socket" ]
runuser -u linux -- env XDG_RUNTIME_DIR=/run/user/1000 WAYLAND_DISPLAY="$(basename "$socket")" grim /run/user/1000/cupid-desktop.png
sha256sum /run/user/1000/cupid-desktop.png
printf '\nCUPID_BASE64_BEGIN\n'
python3 - <<'PY'
import base64
import sys
import time
with open('/run/user/1000/cupid-desktop.png', 'rb') as f:
    while chunk := f.read(1536):
        sys.stdout.write(base64.b64encode(chunk).decode() + '\n')
        sys.stdout.flush()
        time.sleep(0.12)
PY
printf 'CUPID_BASE64_END\n'
