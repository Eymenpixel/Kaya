# Small Creatures

Currently supports Ubuntu/WSL APT only:

- `small-creatures --add APT` safely links the existing host `apt` executable to `~/.local/bin/apt`.
- `small-creatures --remove APT` removes only that link.
- `small-creatures --list` shows whether the link exists.

This is a PATH integration helper, not an APT installer. It never copies or
replaces Ubuntu's package manager and leaves Kaya installed.
