#!/bin/sh
set -u

if ! command -v curl >/dev/null 2>&1; then
    printf 'curl is not installed on this host.\n' >&2
    exit 127
fi

printf 'Checking GitHub API connectivity...\n'
curl -fsS --max-time 10 -o /dev/null \
    -w 'GitHub API responded with HTTP %{http_code}.\n' \
    https://api.github.com/rate_limit
