#!/usr/bin/env python3
import argparse
import time

parser = argparse.ArgumentParser(description="Run a countdown timer.")
parser.add_argument("seconds", type=float, help="duration in seconds")
parser.add_argument("--repeat", type=int, default=1, help="number of countdowns")
args = parser.parse_args()
if args.seconds <= 0 or args.seconds > 7 * 24 * 60 * 60:
    parser.error("seconds must be greater than zero and at most seven days")
if not 1 <= args.repeat <= 100:
    parser.error("repeat must be between 1 and 100")

try:
    for round_no in range(1, args.repeat + 1):
        end = time.monotonic() + args.seconds
        while True:
            remaining = end - time.monotonic()
            if remaining <= 0:
                break
            minutes, seconds = divmod(int(remaining + .999), 60)
            print(f"\r{minutes:02d}:{seconds:02d}  ", end="", flush=True)
            time.sleep(min(.25, remaining))
        print(f"\rTimer {round_no}/{args.repeat} finished.        ", flush=True)
except KeyboardInterrupt:
    print("\nTimer stopped.")
