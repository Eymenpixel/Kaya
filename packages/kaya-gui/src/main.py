#!/usr/bin/env python3
"""Kaya-GUI starter: a small Tk front-end for Kaya's CLI."""

import os
from pathlib import Path
import shutil
import subprocess
import tkinter as tk
from tkinter import filedialog, messagebox


def main() -> int:
    kaya = os.environ.get("KAYA_BIN") or shutil.which("kaya")
    if not kaya:
        messagebox.showerror("Kaya-GUI", "The Kaya command was not found on PATH.")
        return 2
    root = tk.Tk()
    root.title("Kaya Package Manager")
    root.geometry("680x420")
    tk.Label(root, text="Installed packages", font=("TkDefaultFont", 14, "bold")).pack(anchor="w", padx=16, pady=(16, 8))
    listing = tk.Listbox(root, height=14, font=("TkFixedFont", 10))
    listing.pack(fill="both", expand=True, padx=16, pady=8)
    output = tk.StringVar(value="Ready")
    tk.Label(root, textvariable=output, anchor="w").pack(fill="x", padx=16, pady=4)

    def refresh() -> None:
        listing.delete(0, "end")
        proc = subprocess.run([kaya, "list"], text=True, capture_output=True)
        if proc.returncode != 0:
            output.set(proc.stderr.strip() or "Could not read Kaya package database")
            return
        for line in proc.stdout.splitlines()[1:]:
            if line.strip(): listing.insert("end", line)
        output.set(f"{listing.size()} package record(s)")

    def install_file() -> None:
        selected = filedialog.askopenfilename(title="Choose a Kaya package", filetypes=(("Kaya packages", "*.kay"), ("All files", "*")))
        if not selected: return
        proc = subprocess.run([kaya, "install", selected], text=True, capture_output=True)
        output.set(proc.stdout.strip() or proc.stderr.strip() or f"Install exited with code {proc.returncode}")
        if proc.returncode == 0: refresh()
        else: messagebox.showerror("Kaya-GUI", output.get())

    def remove_selected() -> None:
        selection = listing.curselection()
        if not selection: return
        name = listing.get(selection[0]).split("\t", 1)[0]
        if not messagebox.askyesno("Remove package", f"Remove {name}?"): return
        proc = subprocess.run([kaya, "remove", name], text=True, capture_output=True)
        output.set(proc.stdout.strip() or proc.stderr.strip() or f"Remove exited with code {proc.returncode}")
        if proc.returncode == 0: refresh()
        else: messagebox.showerror("Kaya-GUI", output.get())

    buttons = tk.Frame(root)
    buttons.pack(fill="x", padx=16, pady=(4, 16))
    tk.Button(buttons, text="Refresh", command=refresh).pack(side="left")
    tk.Button(buttons, text="Install .kay…", command=install_file).pack(side="left", padx=8)
    tk.Button(buttons, text="Remove selected", command=remove_selected).pack(side="left")
    root.after(100, refresh)
    root.mainloop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
