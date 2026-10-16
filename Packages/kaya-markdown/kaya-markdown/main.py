#!/usr/bin/env python3
import argparse
import re
import sys
import textwrap
from pathlib import Path

parser = argparse.ArgumentParser(description="Render basic Markdown as readable terminal text.")
parser.add_argument("file", help="Markdown file to read")
args = parser.parse_args()
try:
    lines = Path(args.file).read_text(encoding="utf-8").splitlines()
except (OSError, UnicodeError) as exc:
    print(f"kaya-markdown: {exc}", file=sys.stderr)
    raise SystemExit(1)

fenced = False
for line in lines:
    stripped = line.strip()
    if stripped.startswith("```") or stripped.startswith("~~~"):
        fenced = not fenced
        print("  " + "─" * 60)
        continue
    if fenced:
        print("  " + line)
        continue
    heading = re.match(r"^(#{1,6})\s+(.*)$", stripped)
    if heading:
        title = re.sub(r"[`*_~]", "", heading.group(2))
        print("\n" + title.upper())
        if len(heading.group(1)) <= 2:
            print("=" * min(len(title), 72))
        continue
    if stripped.startswith("> "):
        print("│ " + stripped[2:])
    elif re.match(r"^[-*+]\s+", stripped):
        print("  • " + re.sub(r"^[-*+]\s+", "", stripped))
    elif re.match(r"^\d+[.)]\s+", stripped):
        print("  " + stripped)
    elif stripped:
        plain = re.sub(r"!?(\[([^]]*)\])\([^)]*\)", r"\2", stripped)
        plain = re.sub(r"[*_~`]", "", plain)
        print("\n".join(textwrap.wrap(plain, width=78)))
    else:
        print()
