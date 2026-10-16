#!/usr/bin/env python3
import argparse
import calendar
from datetime import date

today = date.today()
parser = argparse.ArgumentParser(description="Display a month calendar.")
parser.add_argument("month", nargs="?", type=int, default=today.month, choices=range(1, 13))
parser.add_argument("year", nargs="?", type=int, default=today.year)
parser.add_argument("--sunday", action="store_true", help="start weeks on Sunday (default: Monday)")
args = parser.parse_args()
cal = calendar.Calendar(calendar.SUNDAY if args.sunday else calendar.MONDAY)
print(f"{calendar.month_name[args.month]} {args.year}\n")
print("Su Mo Tu We Th Fr Sa" if args.sunday else "Mo Tu We Th Fr Sa Su")
for week in cal.monthdayscalendar(args.year, args.month):
    print(" ".join(f"{day:2}" if day else "  " for day in week))
