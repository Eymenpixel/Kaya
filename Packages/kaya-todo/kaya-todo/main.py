#!/usr/bin/env python3
import json
import os
import sys
from pathlib import Path

data_dir = os.environ.get("KAYA_DATA_DIR")
if not data_dir:
    data_dir = str(Path.home() / ".local" / "share" / "kaya" / "todo")
store = Path(data_dir) / "tasks.json"


def load():
    try:
        return json.loads(store.read_text(encoding="utf-8"))
    except FileNotFoundError:
        return []
    except (OSError, json.JSONDecodeError) as exc:
        print(f"kaya-todo: cannot read {store}: {exc}", file=sys.stderr)
        raise SystemExit(1)


def save(tasks):
    try:
        store.parent.mkdir(parents=True, exist_ok=True)
        temporary = store.with_suffix(".tmp")
        temporary.write_text(json.dumps(tasks, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        temporary.replace(store)
    except OSError as exc:
        print(f"kaya-todo: cannot save {store}: {exc}", file=sys.stderr)
        raise SystemExit(1)


def usage():
    print("Usage: kaya-todo add <task> | list | done <id> | remove <id> | clear")


args = sys.argv[1:]
tasks = load()
if not args or args[0] == "list":
    if not tasks:
        print("No tasks yet. Add one with: kaya-todo add <task>")
    for task in tasks:
        mark = "x" if task["done"] else " "
        print(f"[{mark}] {task['id']}. {task['text']}")
elif args[0] == "add" and len(args) >= 2:
    text = " ".join(args[1:]).strip()
    if not text:
        usage()
        raise SystemExit(2)
    next_id = max((task["id"] for task in tasks), default=0) + 1
    tasks.append({"id": next_id, "text": text, "done": False})
    save(tasks)
    print(f"Added task {next_id}.")
elif args[0] in {"done", "remove"} and len(args) == 2:
    try:
        task_id = int(args[1])
    except ValueError:
        usage()
        raise SystemExit(2)
    task = next((task for task in tasks if task["id"] == task_id), None)
    if task is None:
        print(f"No task with ID {task_id}.", file=sys.stderr)
        raise SystemExit(1)
    if args[0] == "done":
        task["done"] = True
        save(tasks)
        print(f"Task {task_id} completed.")
    else:
        save([item for item in tasks if item["id"] != task_id])
        print(f"Task {task_id} removed.")
elif args[0] == "clear" and len(args) == 1:
    save([])
    print("All tasks cleared.")
else:
    usage()
    raise SystemExit(2)
