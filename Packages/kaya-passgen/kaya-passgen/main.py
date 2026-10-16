#!/usr/bin/env python3
import argparse
import secrets
import string

parser = argparse.ArgumentParser(description="Generate a random password locally.")
parser.add_argument("length", nargs="?", type=int, default=24, help="length (4..256, default 24)")
parser.add_argument("--no-symbols", action="store_true", help="use letters and digits only")
args = parser.parse_args()
if not 4 <= args.length <= 256:
    parser.error("length must be between 4 and 256")

groups = [string.ascii_lowercase, string.ascii_uppercase, string.digits]
if not args.no_symbols:
    groups.append("!#$%&*+-=?@^_~")
alphabet = "".join(groups)
password = [secrets.choice(group) for group in groups]
password.extend(secrets.choice(alphabet) for _ in range(args.length - len(password)))
secrets.SystemRandom().shuffle(password)
print("".join(password))
