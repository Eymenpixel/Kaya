#!/bin/sh
set -u

printf '\nKaya Doctor\n'
printf '%s\n' '------------'
missing=0
for tool in curl tar python3; do
    if command -v "$tool" >/dev/null 2>&1; then
        printf '[OK]      %s\n' "$tool"
    else
        printf '[MISSING] %s\n' "$tool"
        missing=1
    fi
done

if [ "$missing" -eq 0 ]; then
    printf '\nThe basic Kaya host tools are available.\n'
    exit 0
fi
printf '\nInstall the missing tools with your Linux distribution package manager.\n'
exit 1
