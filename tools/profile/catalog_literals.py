"""Function and SymbolBinding literals of the plugin catalogs.

The catalogs under include/ubc spell every lhiver::Function as
{"NAME", symbol, byte_offset, byte_length, digest_literal("..."), ...} and
every lhiver::SymbolBinding as ten plain fields. The scanner reads only the
repository sources; a generated profile must carry a record for each one.
"""

from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path

_STRING = r'"((?:[^"\\]|\\.)*)"(?:\s*"((?:[^"\\]|\\.)*)")?'
_INT = r"\s*(-?\d+)\s*"
FUNCTION = re.compile(
    r"\{\s*"
    + _STRING
    + r"\s*,"
    + _INT
    + ","
    + _INT
    + ","
    + _INT
    + r",\s*(?:\w+::)*digest_literal\(\s*"
    r'"([0-9a-f]{64})"\s*\)'
    r"(?:\s*,(?P<rest>[\s\d,-]*?))?\s*\}"
)
BINDING = re.compile(
    r"\{\s*"
    + _STRING
    + r"\s*,"
    + ",".join([_INT] * 5)
    + r",\s*(true|false)\s*,"
    + _INT
    + r",\s*(true|false)\s*,"
    + _INT
    + r"\}"
)
_GENERATED = "generated"


def _name(first: str, second: str | None) -> str:
    text = first + (second or "")
    out = bytearray()
    i = 0
    while i < len(text):
        if text[i] == "\\" and i + 1 < len(text):
            if text[i + 1] == "x":
                j = i + 2
                while (
                    j < len(text) and j < i + 4 and text[j] in "0123456789abcdefABCDEF"
                ):
                    j += 1
                out.append(int(text[i + 2 : j], 16))
                i = j
                continue
            out.extend(text[i + 1].encode("latin-1"))
            i += 2
            continue
        out.extend(text[i].encode("latin-1"))
        i += 1
    return out.decode("latin-1")


@dataclass(frozen=True)
class FunctionLiteral:
    name: str
    symbol: int
    start: int
    length: int
    raw_sha256: str
    argc: int
    type: int
    flags: int
    return_type: int
    parent: int
    source: str


@dataclass(frozen=True)
class BindingLiteral:
    name: str
    symbol: int
    type: int
    flags: int
    elements: int
    parent: int
    check_value: bool
    value: int
    check_offset: bool
    offset: int
    source: str


def scan(include: Path) -> tuple[list[FunctionLiteral], list[BindingLiteral]]:
    functions: list[FunctionLiteral] = []
    bindings: list[BindingLiteral] = []
    for path in sorted(include.rglob("*.hpp")):
        relative = path.relative_to(include.parent).as_posix()
        if _GENERATED in path.relative_to(include).parts:
            continue
        text = path.read_text(encoding="utf-8")
        for match in FUNCTION.finditer(text):
            rest = [
                int(x)
                for x in (match["rest"] or "").replace("\n", " ").split(",")
                if x.strip()
            ]
            argc, kind, flags, return_type, parent = (
                rest + [0, 5, 1, 0, -1][len(rest) :]
            )[:5]
            functions.append(
                FunctionLiteral(
                    _name(match[1], match[2]),
                    int(match[3]),
                    int(match[4]),
                    int(match[5]),
                    match[6],
                    argc,
                    kind,
                    flags,
                    return_type,
                    parent,
                    relative,
                )
            )
        for match in BINDING.finditer(text):
            bindings.append(
                BindingLiteral(
                    _name(match[1], match[2]),
                    *(int(match[i]) for i in range(3, 8)),
                    match[8] == "true",
                    int(match[9]),
                    match[10] == "true",
                    int(match[11]),
                    relative,
                )
            )
    return functions, bindings
