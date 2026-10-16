#!/usr/bin/env python3
"""Small calculator that evaluates arithmetic syntax only."""
import ast
import operator
import sys

OPS = {
    ast.Add: operator.add, ast.Sub: operator.sub, ast.Mult: operator.mul,
    ast.Div: operator.truediv, ast.FloorDiv: operator.floordiv,
    ast.Mod: operator.mod, ast.Pow: operator.pow,
    ast.UAdd: operator.pos, ast.USub: operator.neg,
}


def calc(node, depth=0):
    if depth > 32:
        raise ValueError("expression is too complex")
    if isinstance(node, ast.Expression):
        return calc(node.body, depth + 1)
    if isinstance(node, ast.Constant) and type(node.value) in (int, float):
        return node.value
    if isinstance(node, ast.BinOp) and type(node.op) in OPS:
        left, right = calc(node.left, depth + 1), calc(node.right, depth + 1)
        if isinstance(node.op, ast.Pow) and abs(right) > 1000:
            raise ValueError("exponent is too large")
        value = OPS[type(node.op)](left, right)
        if isinstance(value, complex) or (isinstance(value, float) and abs(value) > 1e100):
            raise ValueError("result is outside the supported range")
        return value
    if isinstance(node, ast.UnaryOp) and type(node.op) in OPS:
        return OPS[type(node.op)](calc(node.operand, depth + 1))
    raise ValueError("only numbers and + - * / // % ** parentheses are allowed")


def evaluate(text):
    return calc(ast.parse(text, mode="eval"))


if len(sys.argv) > 1:
    print(evaluate(" ".join(sys.argv[1:])))
else:
    print("Kaya Calculator — type q to quit")
    while True:
        try:
            text = input("calc> ").strip()
            if text.lower() in {"q", "quit", "exit"}:
                break
            if text:
                print(evaluate(text))
        except (ValueError, SyntaxError, ZeroDivisionError, OverflowError) as exc:
            print(f"Error: {exc}", file=sys.stderr)
        except (EOFError, KeyboardInterrupt):
            print()
            break
