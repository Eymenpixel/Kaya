# Kaya package repository

This directory is the default package channel for Kaya on Nexora OS.

## Repository layout

```text
packages/
  index.json
  <package-id>/
    <package-id>.kay       distributable PAX tar archive
    manifest.json          source manifest
    README.md              package-specific notes
    src/                   package source, when applicable
```

Every `.kay` archive contains its `manifest.json` at the archive root. Package IDs use lowercase letters, digits, dots, underscores, and hyphens. Versions use numeric dotted components, optionally followed by a prerelease or build suffix.

## Manifest v1

Application package example:

```json
{
  "schema_version": 1,
  "name": "example-app",
  "version": "0.1.0",
  "description": "Short package summary",
  "runtime": "python",
  "entry": "src/main.py",
  "depends": ["kaya-core>=0.1.0"],
  "conflicts": [],
  "system_dependencies": ["python3 >= 3.10"],
  "commands": ["example-app"]
}
```

`runtime` is `cpp`, `python`, or `wasm` for an application. A `meta` package has no `entry`; it declares Kaya package dependencies and/or Nexora system dependencies without shipping an executable. `depends` entries use forms such as `kaya-core`, `kaya-core>=0.1.0`, or `image-tools<2.0.0`. `commands` names the command Kaya should expose on Nexora's `PATH` after installation.

`system_dependencies` are requirements supplied by Nexora, not fake `.kay` packages. They document the host runtime, API, or tool that the package needs. The target system's package adapter will resolve those names.

## Current package set

The three planned user-facing applications are Small-Creatures, Nexus, and Kaya-GUI. The other packages are dependency declarations for Kaya/UST, C++ and Python runtimes, terminal/GUI APIs, and VM/image backends. Nexus's broad VM format goal is adapter-based; actual support depends on the installed backend and Nexora APIs.

These are early package starters. A package may describe planned integrations; its package README must say so plainly. Do not bundle downloaded third-party executables or claim that a dependency declaration installs an external tool by itself.

## Repository channels

- Default: `https://github.com/Eymenpixel/Kaya/tree/main/packages`
- Fan/community: `https://github.com/Eymenpixel/Kaya-Fan/tree/main/packages` (`--fan`)

The intended install forms are `kaya install <package-name>` and `kaya install <package-name> --fan`.

The machine-readable catalog is [`index.json`](index.json). Update its package entries and SHA-256 values whenever a `.kay` archive changes.
