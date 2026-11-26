"""Read-only reader for compiled Daedalus scripts (Gothic II GOTHIC.DAT).

Layout of the file, all integers little-endian:

    u8 version, u32 count, u32[count] sort order, count symbols, u32 code
    length, code bytes

Each symbol starts with u32 has_name and, when set, the name up to a line
feed. Then follow i32 offset (class member offset, class size or FUNC
return type), u32 packed elements | type << 12 | flags << 16, five i32
source positions, the content unless the symbol is a class member, and
i32 parent. Content is elements int32 or float32 values, elements strings
up to a line feed, or one int32 for class, function, prototype and
instance symbols.

Names stay raw bytes; this module exposes them as Latin-1 strings, which
round-trip every byte and sort exactly like unsigned bytes. String contents
stay the exact file bytes; nothing is translated. The file is only
read; nothing here writes or executes game data.
"""

from __future__ import annotations

import hashlib
import struct
from bisect import bisect_right
from dataclasses import dataclass, field
from pathlib import Path

VOID, FLOAT, INT, STRING, CLASS, FUNC, PROTOTYPE, INSTANCE = range(8)
CONST, RETURN, CLASSVAR, EXTERNAL, MERGED = 1, 2, 4, 8, 16
CODE_TYPES = (FUNC, PROTOTYPE, INSTANCE)

OP_RET, OP_CALL, OP_CALL_EXTERNAL, OP_PUSH_INT = 60, 61, 62, 64
OP_JUMP, OP_JUMPF, OP_PUSH_ARRAY = 75, 76, 245
SYMBOL_OPERANDS = {62, 65, 66, 67, 68, 80, 245}
_WORD = {61, 62, 64, 65, 66, 67, 68, 75, 76, 80}
_SIMPLE = (
    set(range(0, 10))
    | set(range(11, 23))
    | set(range(30, 34))
    | {60, 63}
    | set(range(69, 75))
)


def instruction_size(opcode: int) -> int:
    """Instruction size in bytes, 0 for an opcode outside the Gothic II table."""
    if opcode in _WORD:
        return 5
    if opcode == OP_PUSH_ARRAY:
        return 6
    return 1 if opcode in _SIMPLE else 0


class DatError(ValueError):
    """The input is not a well-formed compiled script."""


@dataclass(slots=True)
class Symbol:
    index: int
    name: str
    offset: int
    elements: int
    type: int
    flags: int
    content: object
    parent: int

    @property
    def anonymous(self) -> bool:
        return self.name.startswith("\xff")

    @property
    def const(self) -> bool:
        return bool(self.flags & CONST)

    @property
    def classvar(self) -> bool:
        return bool(self.flags & CLASSVAR)

    @property
    def external(self) -> bool:
        return bool(self.flags & EXTERNAL)

    @property
    def returns(self) -> bool:
        return bool(self.flags & RETURN)

    @property
    def address(self) -> int | None:
        """Code start of a script function, prototype or instance."""
        if (
            self.type in CODE_TYPES
            and not self.external
            and isinstance(self.content, int)
        ):
            return self.content
        return None


@dataclass(frozen=True, slots=True)
class Instruction:
    offset: int
    opcode: int
    size: int
    operand: int = 0
    element: int = 0


@dataclass
class DatFile:
    version: int
    symbols: list[Symbol]
    code: bytes
    size: int
    sha256: str
    by_name: dict[str, Symbol] = field(default_factory=dict)
    starts: list[int] = field(default_factory=list)
    entries: dict[int, list[Symbol]] = field(default_factory=dict)

    @classmethod
    def load(cls, path: str | Path) -> DatFile:
        return cls.parse(Path(path).read_bytes())

    @classmethod
    def parse(cls, data: bytes) -> DatFile:
        try:
            return cls._parse(data)
        except (struct.error, IndexError) as error:
            raise DatError(f"truncated script: {error}") from error

    @classmethod
    def _parse(cls, data: bytes) -> DatFile:
        position = 0

        def take(fmt: str):
            nonlocal position
            values = struct.unpack_from(fmt, data, position)
            position += struct.calcsize(fmt)
            return values

        def line() -> bytes:
            nonlocal position
            end = data.index(b"\n", position)
            value = data[position:end]
            position = end + 1
            return value

        (version,) = take("<B")
        (count,) = take("<I")
        if count > len(data) // 4:
            raise DatError("symbol count exceeds the file")
        position += 4 * count
        symbols: list[Symbol] = []
        for index in range(count):
            (has_name,) = take("<I")
            name = line().decode("latin-1") if has_name else ""
            offset, packed, *_ = take("<iIiiiii")
            elements = packed & 0xFFF
            kind = (packed >> 12) & 0xF
            flags = (packed >> 16) & 0x3F
            content: object = None
            if not flags & CLASSVAR:
                if kind == FLOAT:
                    content = list(take(f"<{elements}f"))
                elif kind == INT:
                    content = list(take(f"<{elements}i"))
                elif kind == STRING:
                    content = [line() for _ in range(elements)]
                elif kind in (CLASS, FUNC, PROTOTYPE, INSTANCE):
                    (content,) = take("<i")
                elif kind != VOID:
                    raise DatError(f"symbol {index}: unknown type {kind}")
            (parent,) = take("<i")
            symbols.append(
                Symbol(index, name, offset, elements, kind, flags, content, parent)
            )
        (length,) = take("<I")
        if position + length != len(data):
            raise DatError("code length does not end the file")
        dat = cls(
            version,
            symbols,
            data[position:],
            len(data),
            hashlib.sha256(data).hexdigest(),
        )
        dat._index()
        return dat

    def _index(self) -> None:
        for symbol in self.symbols:
            if symbol.name and not symbol.anonymous:
                self.by_name.setdefault(symbol.name, symbol)
            address = symbol.address
            if address is not None and 0 <= address < len(self.code):
                self.entries.setdefault(address, []).append(symbol)
        self.starts = sorted(self.entries)

    def __getitem__(self, name: str) -> Symbol:
        return self.by_name[name]

    def entry(self, address: int) -> Symbol | None:
        """The one code symbol starting exactly at address, if unique."""
        found = self.entries.get(address, ())
        return found[0] if len(found) == 1 else None

    def extent(self, symbol: Symbol) -> tuple[int, int] | None:
        """[start, end) up to the next code entry, None without own code."""
        start = symbol.address
        if start is None or start not in self.entries:
            return None
        position = bisect_right(self.starts, start)
        end = self.starts[position] if position < len(self.starts) else len(self.code)
        return start, end

    def instructions(self, start: int, end: int) -> list[Instruction]:
        result = []
        position = start
        while position < end:
            opcode = self.code[position]
            size = instruction_size(opcode)
            if size == 0 or position + size > end:
                raise DatError(f"invalid instruction at {position}")
            operand = (
                struct.unpack_from("<i", self.code, position + 1)[0] if size > 1 else 0
            )
            element = self.code[position + 5] if size == 6 else 0
            result.append(Instruction(position, opcode, size, operand, element))
            position += size
        return result

    def class_of(self, index: int) -> str:
        """Name of the class an instance or prototype derives from."""
        symbol = self.symbols[index]
        for _ in range(16):
            if symbol.type == CLASS:
                return symbol.name
            if not 0 <= symbol.parent < len(self.symbols):
                break
            symbol = self.symbols[symbol.parent]
        return ""

    def parent_name(self, symbol: Symbol) -> str:
        if 0 <= symbol.parent < len(self.symbols):
            return self.symbols[symbol.parent].name
        return ""
