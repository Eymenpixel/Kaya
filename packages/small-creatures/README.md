# Small-Creatures

Small-Creatures is the planned terminal-personalization package for Nexora. It provides a command-line interface and a small GUI for editing terminal background, script colors, and font preferences.

The current starter writes a user preference file at `~/.config/nexora/terminal-theme.json` (or the path in `NEXORA_TERMINAL_THEME`). The live terminal will apply these settings once Nexora's terminal profile API is available; this package does not patch the kernel or shell directly.

Launch the GUI with `small-creatures gui`. CLI examples:

```sh
small-creatures show
small-creatures set --background '#101820' --script-color '#e8f1ff' --font 'MiSans'
```
