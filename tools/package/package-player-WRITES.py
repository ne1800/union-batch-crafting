#!/usr/bin/env python3
"""Assemble the player ZIP from the dedicated Windows x86 target."""

import argparse
from pathlib import Path
import os
import struct
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[2]
DLL = "UnionBatchCraftingExtended.dll"
VERSION = (ROOT / "VERSION").read_text(encoding="utf-8").strip()
ARCHIVE = f"union-batch-crafting-player-{VERSION}.zip"
DOCUMENTS = (
    "INSTALL.md",
    "INSTALL.de.md",
    "TESTED-WITH.md",
    "LICENSE",
    "THIRD-PARTY-NOTICES",
)


def validate_dll(data, version=VERSION):
    if data[:2] != b"MZ" or len(data) < 64:
        raise ValueError("Missing DOS header")
    pe = struct.unpack_from("<I", data, 60)[0]
    if (
        pe < 64
        or pe + 26 > len(data)
        or data[pe : pe + 4] != b"PE\0\0"
        or struct.unpack_from("<H", data, pe + 4)[0] != 0x14C
        or not struct.unpack_from("<H", data, pe + 22)[0] & 0x2000
        or struct.unpack_from("<H", data, pe + 24)[0] != 0x10B
    ):
        raise ValueError("Expected a Windows x86 DLL")
    if (
        (version + "\0").encode("utf-16le") not in data
        or b"EXTENDED_AMOUNTS_V1" in data
        or ".enable-once".encode("utf-16le") in data
        or "Enabled\0".encode("utf-16le") not in data
    ):
        raise ValueError(f"Expected the permanent {version} player build")


def package(root):
    dll = (root / "build/windows/player" / DLL).read_bytes()
    validate_dll(dll)
    files = {name: (root / name).read_bytes() for name in DOCUMENTS}
    files["system/autorun/" + DLL] = dll
    folder = root / "dist"
    if folder.resolve() != folder:
        raise ValueError("Linked output directory")
    folder.mkdir(exist_ok=True)
    with tempfile.NamedTemporaryFile(dir=folder, delete=False) as output:
        temp = Path(output.name)
    try:
        with zipfile.ZipFile(temp, "w") as archive:
            for name, data in sorted(files.items()):
                info = zipfile.ZipInfo(name, (1980, 1, 1, 0, 0, 0))
                info.external_attr = 0o100644 << 16
                archive.writestr(info, data, zipfile.ZIP_DEFLATED, compresslevel=9)
        os.chmod(temp, 0o644)
        os.replace(temp, folder / ARCHIVE)
    finally:
        temp.unlink(missing_ok=True)
    return folder / ARCHIVE


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--apply", action="store_true")
    print(
        package(ROOT) if parser.parse_args().apply else "Preview: build dist/" + ARCHIVE
    )
