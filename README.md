# Kaya packages

This repository contains Kaya packages under `Packages/` and Kaya plugins under
`Plugins/`. Each package folder should contain one `.kay` tar archive. That
archive must have `manifest.json` and the manifest's entry file at its root.

## Packages

- `blackbit-info` — read-only host OS and uptime summary.
- `kaya-doctor` — read-only check for `curl`, `tar`, and `python3`.
- `netcheck` — makes one read-only request to the GitHub API using `curl`.
- `kaya-calc` - arithmetic-only calculator (no expression execution).
- `kaya-passgen` - local cryptographic password generator.
- `kaya-json` - JSON validation and formatting.
- `kaya-hash` - SHA-256, SHA-512, and BLAKE2b file checksums.
- `kaya-todo` - persistent per-app task list.
- `kaya-units` - length, mass, and temperature conversion.
- `kaya-serve` - local web server, bound to localhost by default.
- `kaya-timer` - countdown timer.
- `kaya-markdown` - terminal Markdown reader.
- `kaya-calendar` - monthly calendar.
- `clock` — existing Kaya clock/widget package.
- `kaya-netdiag` - interface, route, DNS, bounded ping, and one TCP port check.
- `kaya-subnet` - IPv4/IPv6 CIDR and subnet calculator.
- `small-creatures` - exposes the host's existing Ubuntu `apt` command on PATH.
- `kaya-edit` - full-screen terminal text editor for UTF-8 files.
- `kaya-browser` - launches an installed Chrome, Chromium, or Firefox browser.

Install a package with `kaya --install <name>`. Kaya adds a command named after
the package to `~/.local/bin`, so it can be run directly, for example
`kaya-calc '12 * 4'`. Existing installations can be linked with
`kaya link --all`. The default package root is `~/.local/share/kaya`.

`small-creatures --add APT` links the existing Ubuntu `apt` executable into
`~/.local/bin`; it does not install or replace apt and does not remove Kaya.
`kaya-netdiag` reuses the host's `ping` command when present; system utilities
are not bundled as duplicate Kaya packages.
`kaya-edit` edits files on the Linux host where Kaya runs. `kaya-browser`
launches a browser already installed on that host; browser binaries are not
bundled in the Kaya repository.

Shell and Python packages run on the Linux host where Kaya is installed; they
do not run inside the freestanding Blackbit kernel. `kaya-serve` binds to
localhost unless the user explicitly requests a network-facing address.
