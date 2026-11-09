#!/usr/bin/env python3
"""Nexus CLI/GUI starter with optional QEMU and VirtualBox backends."""

import argparse
import json
from pathlib import Path
import shutil
import subprocess
import sys

KNOWN = {".iso", ".img", ".raw", ".ext4", ".qcow2", ".vdi", ".vmdk", ".vhd", ".vhdx"}


def backends() -> int:
    for tool in ("qemu-system-x86_64", "qemu-img", "VBoxManage"):
        print(f"{tool}: {shutil.which(tool) or 'not installed'}")
    return 0


def inspect(path_text: str) -> int:
    path = Path(path_text).expanduser()
    if not path.is_file():
        print(f"nexus: file not found: {path}", file=sys.stderr)
        return 2
    suffix = path.suffix.lower()
    print(f"File: {path}\nExtension: {suffix or '(none)'}\nRecognized: {'yes' if suffix in KNOWN else 'unknown'}")
    qemu_img = shutil.which("qemu-img")
    if qemu_img and suffix not in {".iso", ".ext4"}:
        result = subprocess.run([qemu_img, "info", "--output=json", str(path)], text=True, capture_output=True)
        if result.returncode == 0:
            try:
                info = json.loads(result.stdout)
                print(f"Image format: {info.get('format', 'unknown')}\nVirtual size: {info.get('virtual-size', 'unknown')}")
            except json.JSONDecodeError:
                print(result.stdout, end="")
        elif result.stderr:
            print(result.stderr.strip(), file=sys.stderr)
    return 0


def start(path_text: str, ram: int) -> int:
    if ram < 256 or ram > 262144:
        print("nexus: --ram must be between 256 and 262144 MiB", file=sys.stderr)
        return 2
    path = Path(path_text).expanduser().resolve()
    if not path.is_file():
        print(f"nexus: file not found: {path}", file=sys.stderr)
        return 2
    qemu = shutil.which("qemu-system-x86_64")
    if not qemu:
        print("nexus: qemu-system-x86_64 is not installed", file=sys.stderr)
        return 2
    if path.suffix.lower() == ".iso":
        command = [qemu, "-m", str(ram), "-cdrom", str(path), "-boot", "d"]
    else:
        fmt = "raw"
        qemu_img = shutil.which("qemu-img")
        if qemu_img:
            result = subprocess.run([qemu_img, "info", "--output=json", str(path)], text=True, capture_output=True)
            if result.returncode == 0:
                try:
                    fmt = json.loads(result.stdout).get("format", "raw")
                except json.JSONDecodeError:
                    pass
        command = [qemu, "-m", str(ram), "-drive", f"file={path},format={fmt},if=virtio"]
    try:
        return subprocess.run(command, check=False).returncode
    except OSError as exc:
        print(f"nexus: could not start QEMU: {exc}", file=sys.stderr)
        return 7


def gui() -> int:
    try:
        import tkinter as tk
        from tkinter import filedialog, messagebox
    except ImportError:
        print("Nexus: Tk GUI support is unavailable; use the CLI.", file=sys.stderr)
        return 2
    root = tk.Tk()
    root.title("Nexus — Virtual Machines")
    root.geometry("600x260")
    selected = tk.StringVar()
    tk.Label(root, text="ISO, IMG, ext4, or virtual disk image").pack(padx=16, pady=(18, 6), anchor="w")
    row = tk.Frame(root)
    row.pack(fill="x", padx=16)
    tk.Entry(row, textvariable=selected).pack(side="left", fill="x", expand=True)
    tk.Button(row, text="Browse…", command=lambda: selected.set(filedialog.askopenfilename())).pack(side="left", padx=(8, 0))
    result = tk.Text(root, height=5, wrap="word")
    result.pack(fill="both", expand=True, padx=16, pady=12)

    def do_inspect() -> None:
        if not selected.get(): return
        result.delete("1.0", "end")
        proc = subprocess.run([sys.executable, str(Path(__file__).resolve()), "inspect", selected.get()], text=True, capture_output=True)
        result.insert("end", proc.stdout + proc.stderr)

    def do_start() -> None:
        if not selected.get(): return
        if messagebox.askyesno("Start VM", "Start this image with QEMU? Only use trusted images."):
            proc = subprocess.Popen([sys.executable, str(Path(__file__).resolve()), "start", selected.get()])
            result.insert("end", f"Started Nexus/QEMU process {proc.pid}\n")

    buttons = tk.Frame(root)
    buttons.pack(fill="x", padx=16, pady=(0, 12))
    tk.Button(buttons, text="Inspect", command=do_inspect).pack(side="left")
    tk.Button(buttons, text="Start with QEMU", command=do_start).pack(side="left", padx=8)
    root.mainloop()
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(prog="nexus")
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("backends")
    p = sub.add_parser("inspect"); p.add_argument("file")
    p = sub.add_parser("start"); p.add_argument("file"); p.add_argument("--ram", type=int, default=2048)
    sub.add_parser("gui")
    args = parser.parse_args()
    if args.command == "backends": return backends()
    if args.command == "inspect": return inspect(args.file)
    if args.command == "start": return start(args.file, args.ram)
    if args.command == "gui": return gui()
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
