# Kaya Text Editor

Run `kaya-edit notes.txt` to open or create a UTF-8 text file.

- Arrow keys move the cursor; Enter inserts a line.
- Ctrl+S saves the file.
- Ctrl+Q exits. Press Ctrl+Q again to discard unsaved changes.

The editor uses Python's standard `curses` module and stores files on the Linux
host where Kaya runs.
