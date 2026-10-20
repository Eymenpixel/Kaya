# Kaya package catalog

This directory contains distributable Kaya applications. A package belongs
here only when it adds an application-level job that Blackbit's built-in
commands do not already perform. Do not ship wrappers that simply rename
`peek`, `scribe`, `treeview`, `machine`, `tasks`, `netview`, `forge`, or another
kernel command.

## Curation rules

- Give every package one clear primary job and a unique package ID.
- Allow at most two packages for the same primary job, and explain the real
  difference in their descriptions.
- Reuse Blackbit's built-in command interface for system operations instead of
  bundling POSIX utilities or assuming a Linux host.
- Declare only the capabilities the application needs. Destructive or
  privileged operations must ask the user at the moment they are performed.
- Include source, a reproducible build recipe, a v2 manifest, and a `.kay`
  archive together in each package directory.
- Mark an application Blackbit-native only after its entry format and API calls
  have been checked against the Blackbit ABI.

## Planned first-party applications

- `small-creatures`: terminal theme editor with color presets and save slots.
- `agent-switcher`: discover and select package-manager adapters supported by
  the running platform.
- `nexus`: virtual-machine front end backed by Blackbit's virtualization API.

These are package specifications, not executable packages yet. The POSIX Kaya
development backend cannot provide the Blackbit ABI or virtualization service.
