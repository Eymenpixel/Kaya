#!/usr/bin/env python3
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

parser = argparse.ArgumentParser(description="Serve files from a directory over HTTP.")
parser.add_argument("directory", nargs="?", default=".")
parser.add_argument("--host", default="127.0.0.1", help="defaults to localhost; use 0.0.0.0 only to share on your LAN")
parser.add_argument("--port", type=int, default=8000)
args = parser.parse_args()
if not 1 <= args.port <= 65535:
    parser.error("port must be between 1 and 65535")
directory = str(Path(args.directory).resolve())
handler = partial(SimpleHTTPRequestHandler, directory=directory)
server = ThreadingHTTPServer((args.host, args.port), handler)
print(f"Serving {directory} at http://{args.host}:{args.port}/ (Ctrl+C to stop)")
try:
    server.serve_forever()
except KeyboardInterrupt:
    print("\nServer stopped.")
finally:
    server.server_close()
