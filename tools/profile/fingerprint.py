"""UBCF1: position-independent fingerprints of Daedalus functions.

Reference implementation of include/ubc/engine/script_fingerprint.hpp. Both
must produce the same digest for the same function. The canonical stream
replaces symbol indices by names, CALL entries by callee names and jump
targets by offsets relative to the function start:

    "UBCF1" 'H' str(name) u32(type) u32(flags) u32(argc) u32(return_type)
    str(parent) u32(byte_length), then per instruction u8(opcode) and
      PUSH_INT literal       'I' i32(value)
      PUSH_INT symbolic      symbol operand
      CALL                   'C' str(callee)
      JUMP, JUMPF            'J' u32(target - start)
      other symbol operands  symbol operand
      PUSH_ARRAY_VAR         symbol operand u8(element)
    symbol operand: 'S' str(name) | 'A' u32(text length) sha256(text) | 'T'

Integers are little-endian; str is a u32 length followed by the bytes.
return_type is the symbol offset for FUNC (type 5) and 0 otherwise. Only the
reference list decides whether a PUSH_INT operand is a symbol index.

Names are raw parser bytes. A str is encoded as Latin-1, which round-trips
every byte; decode DAT names with Latin-1 (not cp1252 with replacement) or
pass bytes. This module reads no game files.
"""

from __future__ import annotations

import hashlib
import struct
from collections.abc import Iterable, Mapping
from dataclasses import dataclass

MAX_FUNCTION_LENGTH = 64 * 1024

OP_RET = 60
OP_CALL = 61
OP_PUSH_INT = 64
OP_JUMP = 75
OP_JUMPF = 76
OP_PUSH_ARRAY = 245

_SIMPLE = (
    set(range(0, 10))
    | set(range(11, 23))
    | set(range(30, 34))
    | {60, 63}
    | set(range(69, 75))
)
_WORD = {61, 62, 64, 65, 66, 67, 68, 75, 76, 80}


def instruction_size(opcode: int) -> int:
    """Instruction size in bytes, 0 for an opcode outside the Gothic II table."""
    if opcode in _WORD:
        return 5
    if opcode == OP_PUSH_ARRAY:
        return 6
    return 1 if opcode in _SIMPLE else 0


@dataclass(frozen=True)
class Text:
    """Content of an anonymous string constant."""

    content: bytes


@dataclass(frozen=True)
class Header:
    name: bytes | str
    type: int = 5
    flags: int = 1
    argc: int = 0
    return_type: int = 0
    parent: bytes | str = b""


@dataclass(frozen=True)
class Result:
    canonical: str
    raw: str
    instructions: int


class FingerprintError(ValueError):
    """The function has no valid UBCF1 form; it must never match."""

    def __init__(self, issue: str, offset: int = 0) -> None:
        super().__init__(f"{issue} at +{offset}")
        self.issue = issue
        self.offset = offset


def _bytes(value: bytes | str) -> bytes:
    return value if isinstance(value, bytes) else value.encode("latin-1")


class _Stream:
    def __init__(self) -> None:
        self.hash = hashlib.sha256()

    def byte(self, value: int) -> None:
        self.hash.update(bytes((value,)))

    def word(self, value: int) -> None:
        self.hash.update(struct.pack("<I", value & 0xFFFFFFFF))

    def text(self, value: bytes | str) -> None:
        data = _bytes(value)
        self.word(len(data))
        self.hash.update(data)


def _symbol_token(
    stream: _Stream,
    symbols: Mapping[int, bytes | str | Text],
    index: int,
    ignore_text: bool,
    offset: int,
) -> None:
    operand = symbols.get(index)
    if operand is None:
        raise FingerprintError("unknown_symbol", offset)
    if isinstance(operand, Text):
        if ignore_text:
            stream.byte(ord("T"))
        else:
            stream.byte(ord("A"))
            stream.word(len(operand.content))
            stream.hash.update(hashlib.sha256(operand.content).digest())
        return
    if not _bytes(operand):
        raise FingerprintError("unknown_symbol", offset)
    stream.byte(ord("S"))
    stream.text(operand)


def fingerprint(
    code: bytes,
    start: int,
    length: int,
    header: Header,
    symbols: Mapping[int, bytes | str | Text],
    entries: Mapping[int, bytes | str],
    symbolic_ints: Iterable[int] = (),
    ignore_text: bool = False,
) -> Result:
    """Canonicalize code[start:start+length] and hash it.

    symbols maps a symbol index to its name or to the Text of an anonymous
    string constant; entries maps a code offset to the function whose entry
    is exactly there. Raises FingerprintError with the C++ issue name.
    """
    listed = list(symbolic_ints)
    if not _bytes(header.name) or any(
        offset >= length or (i and offset <= listed[i - 1])
        for i, offset in enumerate(listed)
    ):
        raise FingerprintError("invalid_input")
    if not 0 < length <= MAX_FUNCTION_LENGTH:
        raise FingerprintError("function_length")
    if start < 0 or start + length > len(code):
        raise FingerprintError("code_unreadable")
    body = code[start : start + length]
    stream = _Stream()
    stream.hash.update(b"UBCF1H")
    stream.text(header.name)
    for value in (header.type, header.flags, header.argc, header.return_type):
        stream.word(value)
    stream.text(header.parent)
    stream.word(length)

    starts: set[int] = set()
    targets: dict[int, int] = {}
    pending = iter(listed)
    next_symbolic = next(pending, None)
    last_opcode = None
    instructions = 0
    position = 0
    while position < length:
        opcode = body[position]
        size = instruction_size(opcode)
        if size == 0:
            raise FingerprintError("unknown_opcode", position)
        if size > length - position:
            raise FingerprintError("truncated_instruction", position)
        if next_symbolic is not None and next_symbolic < position:
            raise FingerprintError("symbolic_int", next_symbolic)
        symbolic = next_symbolic == position
        if symbolic and opcode != OP_PUSH_INT:
            raise FingerprintError("symbolic_int", position)
        starts.add(position)
        stream.byte(opcode)
        operand = struct.unpack_from("<I", body, position + 1)[0] if size > 1 else 0
        if opcode == OP_PUSH_INT:
            if symbolic:
                next_symbolic = next(pending, None)
                _symbol_token(stream, symbols, operand, ignore_text, position)
            else:
                stream.byte(ord("I"))
                stream.word(operand)
        elif opcode == OP_CALL:
            callee = entries.get(operand)
            if callee is None or not _bytes(callee):
                raise FingerprintError("unknown_callee", position)
            stream.byte(ord("C"))
            stream.text(callee)
        elif opcode in (OP_JUMP, OP_JUMPF):
            relative = operand - start
            if not 0 <= relative < length:
                raise FingerprintError("jump_target", position)
            targets.setdefault(relative, position)
            stream.byte(ord("J"))
            stream.word(relative)
        elif size > 1:
            _symbol_token(stream, symbols, operand, ignore_text, position)
            if opcode == OP_PUSH_ARRAY:
                stream.byte(body[position + 5])
        last_opcode = opcode
        instructions += 1
        position += size
    if next_symbolic is not None:
        raise FingerprintError("symbolic_int", next_symbolic)
    if last_opcode != OP_RET:
        raise FingerprintError("missing_ret", length)
    stray = sorted(target for target in targets if target not in starts)
    if stray:
        raise FingerprintError("jump_target", stray[0])
    return Result(
        stream.hash.hexdigest(), hashlib.sha256(body).hexdigest(), instructions
    )
