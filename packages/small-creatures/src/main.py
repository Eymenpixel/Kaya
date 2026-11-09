#!/usr/bin/env python3
"""Small-Creatures terminal preference editor starter."""

import argparse
import json
import os
from pathlib import Path
import tempfile


def config_path() -> Path:
    override = os.environ.get("NEXORA_TERMINAL_THEME")
    if override:
        return Path(override).expanduser()
    return Path.home() / ".config/nexora/terminal-theme.json"


def read_config() -> dict:
    path = config_path()
    if not path.exists():
        return {"background": "#101820", "script_color": "#e8f1ff", "font": "monospace"}
    return json.loads(path.read_text(encoding="utf-8"))


def write_config(config: dict) -> None:
    path = config_path()
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, temp_name = tempfile.mkstemp(prefix=".small-creatures-", dir=path.parent)
    try:
        with os.fdopen(fd, "w", encoding="utf-8") as stream:
            json.dump(config, stream, indent=2)
            stream.write("\n")
        os.replace(temp_name, path)
    finally:
        if os.path.exists(temp_name):
            os.unlink(temp_name)


def gui() -> int:
    try:
        import tkinter as tk
        from tkinter import messagebox
    except ImportError:
        print("Small-Creatures: Tk GUI support is unavailable; use the CLI.")
        return 2

    values = read_config()
    root = tk.Tk()
    root.title("Small-Creatures — Terminal Style")
    root.geometry("420x260")
    entries = {}
    for row, (key, title) in enumerate((("background", "Background"), ("script_color", "Script color"), ("font", "Font"))):
        tk.Label(root, text=title).grid(row=row, column=0, padx=12, pady=12, sticky="w")
        entry = tk.Entry(root, width=32)
        entry.insert(0, values.get(key, ""))
        entry.grid(row=row, column=1, padx=12, pady=12)
        entries[key] = entry

    def save() -> None:
        values.update({key: widget.get().strip() for key, widget in entries.items()})
        write_config(values)
        messagebox.showinfo("Small-Creatures", f"Preferences saved to {config_path()}")

    tk.Button(root, text="Save preferences", command=save).grid(row=3, column=1, padx=12, pady=14, sticky="e")
    root.mainloop()
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description="Customize Nexora terminal preferences")
    sub = parser.add_subparsers(dest="action", required=True)
    sub.add_parser("show", help="show current preferences")
    set_cmd = sub.add_parser("set", help="update preferences")
    set_cmd.add_argument("--background")
    set_cmd.add_argument("--script-color")
    set_cmd.add_argument("--font")
    sub.add_parser("gui", help="open the graphical editor")
    args = parser.parse_args()
    if args.action == "gui":
        return gui()
    if args.action == "show":
        print(json.dumps(read_config(), indent=2))
        return 0
    config = read_config()
    changes = {"background": args.background, "script_color": args.script_color, "font": args.font}
    config.update({key: value for key, value in changes.items() if value is not None})
    write_config(config)
    print(f"Saved terminal preferences to {config_path()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
