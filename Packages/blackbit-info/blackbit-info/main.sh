#!/bin/sh
set -eu

printf '\nBlackbit System Info\n'
printf '%s\n' '-------------------'
printf 'Kernel: '
uname -sr 2>/dev/null || printf '%s\n' 'unknown'
printf 'Architecture: '
uname -m 2>/dev/null || printf '%s\n' 'unknown'
if command -v uptime >/dev/null 2>&1; then
    printf 'Uptime: '
    uptime -p 2>/dev/null || uptime
fi
if [ -r /etc/os-release ]; then
    . /etc/os-release
    printf 'Distribution: %s\n' "${PRETTY_NAME:-Linux}"
fi
printf '\nThis reports the Kaya host environment, not the Blackbit guest kernel.\n'
