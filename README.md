# Kaya

**The Great Return of Kaya — the package manager for Nexora OS.**

Kaya is being rebuilt as Nexora's native package manager. Its `.kay` packages are intended to make system components and applications easy to discover, install, update, and remove. This repository is the community package source for the default Kaya channel.

> **Early development:** package formats, Nexora system APIs, and application integrations are still being built. The packages in this repository are starter packages and dependency declarations; they are not a claim that every planned feature is already available.

## Main packages

| Package | Purpose |
| --- | --- |
| **Small-Creatures** | Terminal personalization from both the CLI and a GUI: background, script colors, and fonts. |
| **Nexus** | A CLI and GUI virtual-machine manager. The goal is to work with ISO, IMG, ext4 disk images, and common VM formats through available backends such as QEMU and VirtualBox. Format and backend support depends on Nexora's system tools. |
| **Kaya-GUI** | A graphical companion to Kaya's command-line package manager. |

## Package channels

Kaya's default package repository is [`Eymenpixel/Kaya/packages`](https://github.com/Eymenpixel/Kaya/tree/main/packages).

Use the `--fan` channel option to select the community fan repository: [`Eymenpixel/Kaya-Fan/packages`](https://github.com/Eymenpixel/Kaya-Fan/tree/main/packages).

The intended command form is `kaya install <package-name>` for the default channel and `kaya install <package-name> --fan` for the fan channel.

Kaya uses **UST (Universal Source Tool)** for package downloads; it does not shell out to `curl`. Remote package installs are intended to verify the package digest before installation.

## `.kay` packages

A `.kay` file is a package archive containing a `manifest.json` and its payload. The manifest records the package ID, version, runtime, entry point, Kaya dependencies, conflicts, and required Nexora system components. See [`packages/README.md`](packages/README.md) and the package catalog in [`packages/index.json`](packages/index.json).

The repository includes Python app starters and meta-packages that describe system dependencies such as the C++ runtime, Python, GUI support, UST, and VM backends. Meta-packages describe requirements; Nexora's system integration is responsible for providing those components.

## Development direction

- C++ Kaya core, with Python integration where needed.
- `.kay` package creation, validation, installation, upgrades, and removal.
- Installed commands exposed through Nexora's `PATH`.
- UST-backed downloads and package integrity checks.
- EMBER protection for the Kaya application itself, planned after the package manager foundation is in place. EMBER is not a package scanner.

## Contributing packages

Start with the package format and contribution notes in [`packages/README.md`](packages/README.md). Keep package IDs lowercase and stable, declare system requirements explicitly, and mark unfinished integrations as planned rather than claiming support that is not implemented yet.

Kaya and the package catalog are licensed under the MIT License. See [`LICENSE`](LICENSE).
