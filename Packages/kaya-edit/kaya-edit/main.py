#!/usr/bin/env python3
import argparse
import curses
import sys
from pathlib import Path


def render(screen, lines, row, col, top, path, dirty, message):
    height, width = screen.getmaxyx()
    screen.erase()
    title = f" Kaya Edit | {path}{' *' if dirty else ''} "
    screen.addnstr(0, 0, title, max(0, width - 1), curses.A_REVERSE)
    visible = max(1, height - 2)
    for y in range(visible):
        index = top + y
        if index < len(lines):
            screen.addnstr(y + 1, 0, lines[index].expandtabs(4), max(0, width - 1))
    footer = message or "Ctrl+S Save   Ctrl+Q Quit   Arrows Move"
    screen.addnstr(height - 1, 0, footer, max(0, width - 1), curses.A_REVERSE)
    screen.move(min(max(1, row - top + 1), max(1, height - 2)), min(col, max(0, width - 1)))
    screen.refresh()


def edit(screen, path):
    curses.curs_set(1)
    try:
        content = path.read_text(encoding="utf-8")
        lines = content.split("\n")
        if lines and lines[-1] == "" and content.endswith("\n"):
            lines.pop()
        if not lines:
            lines = [""]
    except FileNotFoundError:
        lines = [""]
    except (OSError, UnicodeError) as exc:
        return f"Cannot open file: {exc}"

    row = col = top = 0
    dirty = False
    message = ""
    quit_armed = False
    while True:
        height, width = screen.getmaxyx()
        if row < top:
            top = row
        if row >= top + max(1, height - 2):
            top = row - max(1, height - 3)
        col = min(col, len(lines[row]))
        render(screen, lines, row, col, top, path, dirty, message)
        message = ""
        key = screen.get_wch()
        if quit_armed and key != chr(17):
            quit_armed = False
        if isinstance(key, str) and ord(key) == 19:
            try:
                path.write_text("\n".join(lines) + "\n", encoding="utf-8")
                dirty = False
                message = "Saved."
            except OSError as exc:
                message = f"Save failed: {exc}"
        elif isinstance(key, str) and ord(key) == 17:
            if dirty:
                if quit_armed:
                    return None
                quit_armed = True
                message = "Unsaved changes: Ctrl+S to save, Ctrl+Q again to discard."
                continue
            return None
        elif key == curses.KEY_UP:
            row = max(0, row - 1)
        elif key == curses.KEY_DOWN:
            row = min(len(lines) - 1, row + 1)
        elif key == curses.KEY_LEFT:
            if col:
                col -= 1
            elif row:
                row -= 1
                col = len(lines[row])
        elif key == curses.KEY_RIGHT:
            if col < len(lines[row]):
                col += 1
            elif row + 1 < len(lines):
                row += 1
                col = 0
        elif key in (curses.KEY_BACKSPACE, "\b", "\x7f"):
            if col:
                lines[row] = lines[row][:col - 1] + lines[row][col:]
                col -= 1
                dirty = True
            elif row:
                col = len(lines[row - 1])
                lines[row - 1] += lines[row]
                del lines[row]
                row -= 1
                dirty = True
        elif key in (curses.KEY_ENTER, "\n", "\r"):
            tail = lines[row][col:]
            lines[row] = lines[row][:col]
            lines.insert(row + 1, tail)
            row += 1
            col = 0
            dirty = True
        elif isinstance(key, str) and key.isprintable():
            lines[row] = lines[row][:col] + key + lines[row][col:]
            col += len(key)
            dirty = True


def main():
    parser = argparse.ArgumentParser(description="Edit a UTF-8 text file in the terminal.")
    parser.add_argument("file", help="file to open or create")
    args = parser.parse_args()
    path = Path(args.file).expanduser()
    if path.exists() and path.is_dir():
        parser.error("the path is a directory")
    try:
        result = curses.wrapper(edit, path)
    except curses.error as exc:
        print(f"Terminal does not support the editor: {exc}", file=sys.stderr)
        return 1
    if result:
        print(result, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
