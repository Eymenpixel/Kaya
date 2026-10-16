#!/usr/bin/env python3
import argparse
import hashlib
import sys

parser = argparse.ArgumentParser(description="Calculate a file's cryptographic hash.")
parser.add_argument("file", help="file to read")
parser.add_argument("--algorithm", choices=("sha256", "sha512", "blake2b"), default="sha256")
args = parser.parse_args()
hasher = hashlib.new(args.algorithm)
try:
    with open(args.file, "rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            hasher.update(block)
except OSError as exc:
    print(f"kaya-hash: {exc}", file=sys.stderr)
    raise SystemExit(1)
print(f"{hasher.hexdigest()}  {args.file}")
