#!/usr/bin/env python3
import argparse
import shutil
import subprocess
import sys
from urllib.parse import urlparse


BROWSERS = {
    "chrome": ("google-chrome", "google-chrome-stable", "chrome"),
    "chromium": ("chromium", "chromium-browser"),
    "firefox": ("firefox",),
}


def main():
    parser = argparse.ArgumentParser(description="Open a URL in an installed desktop browser.")
    parser.add_argument("url", nargs="?", default="https://www.google.com", help="http(s) URL to open")
    parser.add_argument("--prefer", choices=("auto", "chrome", "chromium", "firefox"), default="auto")
    parser.add_argument("--list", action="store_true", help="list supported browsers found on PATH")
    args = parser.parse_args()

    found = {name: next((shutil.which(binary) for binary in choices if shutil.which(binary)), None)
             for name, choices in BROWSERS.items()}
    if args.list:
        for name, path in found.items():
            print(f"{name}: {path or 'not installed'}")
        return 0

    parsed = urlparse(args.url)
    if parsed.scheme not in ("http", "https") or not parsed.netloc:
        parser.error("URL must start with http:// or https://")

    order = ("chrome", "chromium", "firefox") if args.prefer == "auto" else (args.prefer,)
    selected = next(((name, found[name]) for name in order if found[name]), None)
    if not selected:
        print("No supported browser is installed. Install Chrome, Chromium, or Firefox on the Linux host first.", file=sys.stderr)
        return 127
    name, executable = selected
    try:
        subprocess.Popen([executable, args.url], stdin=subprocess.DEVNULL,
                         stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                         start_new_session=True)
    except OSError as exc:
        print(f"Could not start {name}: {exc}", file=sys.stderr)
        return 1
    print(f"Opened {args.url} with {name} ({executable}).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
