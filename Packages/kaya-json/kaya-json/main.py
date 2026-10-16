#!/usr/bin/env python3
import argparse
import json
import sys

parser = argparse.ArgumentParser(description="Validate and format JSON.")
parser.add_argument("file", nargs="?", help="JSON file; reads standard input when omitted")
parser.add_argument("--compact", action="store_true", help="print compact JSON")
parser.add_argument("--sort-keys", action="store_true", help="sort object keys")
args = parser.parse_args()

try:
    if args.file:
        with open(args.file, "r", encoding="utf-8") as source:
            value = json.load(source)
    else:
        value = json.load(sys.stdin)
    if args.compact:
        print(json.dumps(value, ensure_ascii=False, sort_keys=args.sort_keys, separators=(",", ":")))
    else:
        print(json.dumps(value, ensure_ascii=False, sort_keys=args.sort_keys, indent=2))
except (OSError, json.JSONDecodeError) as exc:
    print(f"kaya-json: {exc}", file=sys.stderr)
    raise SystemExit(1)
