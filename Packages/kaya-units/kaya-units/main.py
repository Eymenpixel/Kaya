#!/usr/bin/env python3
import argparse
import sys

parser = argparse.ArgumentParser(description="Convert common units.")
parser.add_argument("value", type=float)
parser.add_argument("from_unit", choices=("m", "km", "cm", "mm", "mi", "ft", "kg", "g", "lb", "c", "f", "k"))
parser.add_argument("to_unit", choices=("m", "km", "cm", "mm", "mi", "ft", "kg", "g", "lb", "c", "f", "k"))
args = parser.parse_args()

length = {"m": 1, "km": 1000, "cm": .01, "mm": .001, "mi": 1609.344, "ft": .3048}
mass = {"kg": 1, "g": .001, "lb": .45359237}
src, dst, value = args.from_unit, args.to_unit, args.value
if src in length and dst in length:
    result = value * length[src] / length[dst]
elif src in mass and dst in mass:
    result = value * mass[src] / mass[dst]
elif src in {"c", "f", "k"} and dst in {"c", "f", "k"}:
    celsius = value if src == "c" else (value - 32) * 5 / 9 if src == "f" else value - 273.15
    if celsius < -273.15:
        print("Temperature is below absolute zero.", file=sys.stderr)
        raise SystemExit(2)
    result = celsius if dst == "c" else celsius * 9 / 5 + 32 if dst == "f" else celsius + 273.15
else:
    parser.error("source and destination units must belong to the same group")
print(f"{value:g} {src} = {result:.8g} {dst}")
