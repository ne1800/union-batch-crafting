"""Sparse zCParser memory image built from a parsed script, for offline tools.

The layout matches what include/ubc/engine/runtime_script.hpp reads: the
parser header at the game address, a symbol pointer table, zCPar_Symbol
objects (+8 name, +0xc name length, +0x18 data, +0x1c offset, +0x20
elements | type << 12 | flags << 16, +0x38 parent), zSTRING objects and
the code. The image file is "UBCI", u32 region count, then per region u32
address, u32 length and the bytes. It holds game data and must stay local.

record() runs tools/profile/record_catalog_reads.cpp on such an image and
maps its heap reads back to the symbols whose zSTRING objects, texts or
arrays were read ("strings" per recipe).
"""

from __future__ import annotations

import bisect
import json
import re
import struct
import subprocess
from pathlib import Path

from datfile import CLASS, CLASSVAR, FLOAT, INT, STRING, DatFile

PARSER = 0x00AB40C0
TABLE, SYMBOLS, NAMES, CODE, HEAP = (
    0x20000000,
    0x21000000,
    0x23000000,
    0x25000000,
    0x28000000,
)
STRIDE = 0x40
# Catalog bindings name anonymous texts as the loaded parser shows them.
_ANONYMOUS_BINDING = re.compile(r'\{\s*"\\xff"\s*"(\d+)"\s*,\s*(\d+)\s*,')


def runtime_anonymous_names(dat: DatFile, include: Path) -> dict[int, bytes]:
    names: dict[int, bytes] = {}
    for path in sorted(include.rglob("*.hpp")):
        for match in _ANONYMOUS_BINDING.finditer(path.read_text(encoding="utf-8")):
            index = int(match[2])
            if index < len(dat.symbols) and dat.symbols[index].anonymous:
                names[index] = b"\xff" + match[1].encode()
    return names


def build(
    dat: DatFile,
    names: dict[int, bytes] | None = None,
    spans: list[tuple[int, int, int]] | None = None,
) -> list[tuple[int, bytes]]:
    """Image regions; spans, when given, receives (begin, end, symbol) of
    every heap allocation (zSTRING objects, texts and arrays)."""
    names = names or {}
    spans = [] if spans is None else spans
    count = len(dat.symbols)
    table = bytearray(4 * count)
    symbols = bytearray(STRIDE * count)
    text = bytearray()
    heap = bytearray()

    def allocate(data: bytes, owner: int) -> int:
        heap.extend(b"\0" * (-len(heap) % 4))
        address = HEAP + len(heap)
        heap.extend(data)
        spans.append((address, address + len(data), owner))
        return address

    for symbol in dat.symbols:
        base = STRIDE * symbol.index
        struct.pack_into("<I", table, 4 * symbol.index, SYMBOLS + base)
        name = names.get(symbol.index, symbol.name.encode("latin-1"))
        name_address = NAMES + len(text)
        text.extend(name + b"\0")
        content = symbol.content
        data = 0
        if symbol.address is not None:
            data = symbol.address
        elif isinstance(content, int) and symbol.type != CLASS:
            data = content & 0xFFFFFFFF
        elif not symbol.flags & CLASSVAR and isinstance(content, list) and content:
            if symbol.type == INT:
                packed = struct.pack(f"<{len(content)}i", *content)
                data = (
                    content[0] & 0xFFFFFFFF
                    if len(content) == 1
                    else allocate(packed, symbol.index)
                )
            elif symbol.type == FLOAT:
                packed = struct.pack(f"<{len(content)}f", *content)
                data = (
                    struct.unpack("<I", packed[:4])[0]
                    if len(content) == 1
                    else allocate(packed, symbol.index)
                )
            elif symbol.type == STRING:
                headers = bytearray()
                for raw in content:
                    value = raw
                    headers += struct.pack(
                        "<IIIII",
                        0,
                        0,
                        allocate(value + b"\0", symbol.index),
                        len(value),
                        len(value) + 1,
                    )
                data = allocate(bytes(headers), symbol.index)
        packed = (
            (symbol.elements & 0xFFF)
            | (symbol.type << 12)
            | ((symbol.flags & 0x3F) << 16)
        )
        parent = SYMBOLS + STRIDE * symbol.parent if 0 <= symbol.parent < count else 0
        struct.pack_into(
            "<IIII", symbols, base + 8, name_address, len(name), len(name) + 1, 0
        )
        struct.pack_into(
            "<III", symbols, base + 0x18, data, symbol.offset & 0xFFFFFFFF, packed
        )
        struct.pack_into("<I", symbols, base + 0x38, parent)
    header = bytearray(0x58)
    struct.pack_into("<III", header, 0x18, TABLE, count, count)
    struct.pack_into(
        "<IIII", header, 0x48, CODE, CODE, CODE + len(dat.code), len(dat.code) + 64
    )
    return [
        (PARSER, bytes(header)),
        (TABLE, bytes(table)),
        (SYMBOLS, bytes(symbols)),
        (NAMES, bytes(text)),
        (CODE, dat.code + bytes(64)),
        (HEAP, bytes(heap)),
    ]


def write(path: Path, regions: list[tuple[int, bytes]]) -> None:
    with path.open("wb") as output:
        output.write(struct.pack("<4sI", b"UBCI", len(regions)))
        for address, data in regions:
            output.write(struct.pack("<II", address, len(data)))
            output.write(data)


def heap_symbols(spans: list[tuple[int, int, int]], reads: list) -> set[int]:
    """Symbols whose heap allocation overlaps one of the [begin, end) reads."""
    ordered = sorted(spans)
    starts = [span[0] for span in ordered]
    found: set[int] = set()
    for begin, end in reads:
        k = max(bisect.bisect_right(starts, begin) - 1, 0)
        while k < len(ordered) and ordered[k][0] < end:
            if ordered[k][1] > begin:
                found.add(ordered[k][2])
            k += 1
    return found


def record(
    recorder: Path,
    campaign: str,
    dat: DatFile,
    image: Path,
    names: dict[int, bytes] | None = None,
) -> list[dict]:
    """Per-recipe verifier reads; the temporary image is removed again."""
    spans: list[tuple[int, int, int]] = []
    write(image, build(dat, names, spans))
    try:
        output = subprocess.run(
            [str(recorder), campaign, str(image)],
            check=True,
            capture_output=True,
            text=True,
        ).stdout
    finally:
        image.unlink()
    recipes = json.loads(output)["recipes"]
    for recipe in recipes:
        recipe["strings"] = sorted(heap_symbols(spans, recipe.pop("heap", [])))
    return recipes
