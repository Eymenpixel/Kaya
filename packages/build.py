#!/usr/bin/env python3
"""Build deterministic .kay archives and refresh the package catalog."""

import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import tarfile
import tempfile

ROOT = Path(__file__).resolve().parent
REPO_URL = "https://github.com/Eymenpixel/Kaya/tree/main/packages"
ID_RE = re.compile(r"^[a-z0-9][a-z0-9._-]{0,63}$")
VERSION_RE = re.compile(r"^[0-9]+(?:\.[0-9]+)*(?:[-+][A-Za-z0-9.]+)?$")
RUNTIMES = {"cpp", "python", "wasm", "meta"}


def safe_rel(value: str) -> PurePosixPath:
    if "\\" in value or "\x00" in value:
        raise ValueError(f"unsafe package path: {value!r}")
    rel = PurePosixPath(value)
    if rel.is_absolute() or not rel.parts or any(part in ("", ".", "..") for part in rel.parts):
        raise ValueError(f"unsafe package path: {value!r}")
    return rel


def validate(folder: Path) -> dict:
    manifest = json.loads((folder / "manifest.json").read_text(encoding="utf-8"))
    if manifest.get("schema_version") != 1:
        raise ValueError(f"{folder.name}: unsupported manifest schema")
    if manifest.get("name") != folder.name or not ID_RE.fullmatch(folder.name):
        raise ValueError(f"{folder.name}: package ID must match its directory name")
    if not VERSION_RE.fullmatch(str(manifest.get("version", ""))):
        raise ValueError(f"{folder.name}: invalid version")
    runtime = manifest.get("runtime")
    if runtime not in RUNTIMES:
        raise ValueError(f"{folder.name}: invalid runtime")
    entry = manifest.get("entry")
    if runtime == "meta":
        if entry is not None:
            raise ValueError(f"{folder.name}: meta package must have a null entry")
    else:
        if not isinstance(entry, str):
            raise ValueError(f"{folder.name}: application package requires an entry")
        entry_path = folder.joinpath(*safe_rel(entry).parts)
        if not entry_path.is_file() or entry_path.is_symlink():
            raise ValueError(f"{folder.name}: missing regular entry file {entry}")
    for field in ("depends", "conflicts", "system_dependencies", "commands"):
        if not isinstance(manifest.get(field, []), list) or any(not isinstance(x, str) for x in manifest.get(field, [])):
            raise ValueError(f"{folder.name}: {field} must be a string array")
    return manifest


def build_archive(folder: Path, manifest: dict) -> tuple[Path, str]:
    output = folder / f"{manifest['name']}.kay"
    fd, temp_name = tempfile.mkstemp(prefix=".kay-build-", dir=folder)
    os.close(fd)
    temp = Path(temp_name)
    try:
        with tarfile.open(temp, mode="w", format=tarfile.PAX_FORMAT) as archive:
            for path in sorted(folder.rglob("*")):
                if path.resolve() in {temp.resolve(), output.resolve()}:
                    continue
                rel = path.relative_to(folder).as_posix()
                safe_rel(rel)
                if path.is_symlink():
                    raise ValueError(f"{folder.name}: symbolic links are not allowed ({rel})")
                if path.is_dir():
                    info = tarfile.TarInfo(rel + "/")
                    info.type = tarfile.DIRTYPE
                    info.mode = 0o755
                    archive.addfile(info)
                elif path.is_file():
                    info = archive.gettarinfo(str(path), arcname=rel)
                    info.uid = info.gid = 0
                    info.uname = info.gname = ""
                    info.mtime = 0
                    info.mode = 0o755 if os.access(path, os.X_OK) else 0o644
                    with path.open("rb") as source:
                        archive.addfile(info, source)
                else:
                    raise ValueError(f"{folder.name}: unsupported file type ({rel})")
        os.replace(temp, output)
    finally:
        temp.unlink(missing_ok=True)
    digest = hashlib.sha256(output.read_bytes()).hexdigest()
    return output, digest


def main() -> None:
    records = []
    for folder in sorted(p for p in ROOT.iterdir() if p.is_dir() and (p / "manifest.json").is_file()):
        manifest = validate(folder)
        archive, digest = build_archive(folder, manifest)
        records.append({
            "name": manifest["name"],
            "version": manifest["version"],
            "runtime": manifest["runtime"],
            "path": archive.relative_to(ROOT).as_posix(),
            "sha256": digest,
            "depends": manifest.get("depends", []),
            "system_dependencies": manifest.get("system_dependencies", []),
            "commands": manifest.get("commands", []),
        })
        print(f"Built {archive.relative_to(ROOT)} [{digest}]")
    (ROOT / "index.json").write_text(json.dumps({"schema_version": 1, "repository": REPO_URL, "packages": records}, indent=2) + "\n", encoding="utf-8")
    print(f"Updated index.json ({len(records)} packages)")


if __name__ == "__main__":
    main()
