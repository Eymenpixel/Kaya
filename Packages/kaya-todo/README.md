# Kaya Tasks

Persistent local task list with `add`, `list`, `done`, `remove`, and `clear`
commands. Data is stored in Kaya's private per-app data directory when Kaya
provides `KAYA_DATA_DIR`; otherwise it uses `~/.local/share/kaya/todo/`.

Examples: `kaya-todo add Finish the project`, `kaya-todo list`,
`kaya-todo done 1`.
