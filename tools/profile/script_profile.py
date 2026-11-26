"""Reference script profile for the tolerant recipe check (UBCF1 relocation).

Input is one reference script, the per-recipe reads of the plugin verifiers
(record_catalog_reads.cpp) and the catalog literals. Output is the data of
include/ubc/engine/script_relocation.hpp: sorted names, function records
with UBCF1 digests, data records and one group per recipe.

Group of a recipe ("medium" width):
  * every function whose code the plugin verifier reads for the recipe,
    wholly or in part; for block profiles (DNdR alchemy and anvil, tobacco)
    minus the owner and choice functions of the other recipes of the same
    block (route and station), which their own groups check;
  * the CALL hull of those functions, stopped at externals, NPC
    constructors and a curated presentation-only list;
  * the station share: functions and data every recipe of the station
    uses, catalog literals that no verifier reads (navigation, categories,
    adapters) and functions that hold texts an adapter reads;
  * data records for every named operand, parent, item and catalog binding;
    a named string constant whose text a verifier reads also gets a text
    record with the length and SHA-256 of that text.
Every code byte a verifier reads must lie in a function of the group, of
the station share or of that block exemption; otherwise the build fails.
PUSH_INT operands count as symbol indices only in slots whose values are
symbol indices throughout the script (stack consumer analysis). Every
anonymous text operand of every function record enters its fingerprint with
the length and SHA-256 of the text (no ignore_text); the relocation reads
those texts through the live operand, since the loaded parser names
anonymous symbols differently than the DAT. Constant INT and FLOAT scalars
are compared by value (FLOAT as its raw 32-bit pattern).

Catalog literals belong to one campaign: sources whose file name starts with
dndr_ describe Night of the Raven, all others Marcello. A literal of the
other campaign can match an index or name by chance; it is ignored and
counted in the summary (foreign_matches). A literal of the campaign that
does not match the reference script, or that reaches no station with
recipes, is an error, and so is a verifier read outside the parser image.
"""

from __future__ import annotations

import hashlib
import math
import re
import struct
from collections import Counter, defaultdict
from dataclasses import dataclass, field

import fingerprint as fp
from catalog_literals import BindingLiteral, FunctionLiteral
from datfile import (
    CLASSVAR,
    EXTERNAL,
    FLOAT,
    FUNC,
    INSTANCE,
    INT,
    OP_CALL,
    OP_CALL_EXTERNAL,
    OP_JUMP,
    OP_JUMPF,
    OP_PUSH_INT,
    OP_RET,
    PROTOTYPE,
    STRING,
    SYMBOL_OPERANDS,
    DatFile,
    Symbol,
)

STATIONS = (
    "stove",
    "alchemy",
    "leather",
    "saw",
    "forge",
    "anvil",
    "bow",
    "cool",
    "grind",
)
ROUTES = (
    "base",
    "station",
    "alchemy_extra",
    "anvil",
    "ore",
    "spirit",
    "bow",
    "cool",
    "grind",
    "tobacco",
    "dndr",
)
BLOCK_ROUTES = {"tobacco", "dndr"}
# Roles in a block that only their own recipe checks.
BLOCK_ROLES = {"owner", "choice"}
# script_fingerprint::max_text_length
MAX_TEXT_LENGTH = 64 * 1024
# Callees reached only for presentation (screen text, voice) or generic
# memory library code: no inventory, attribute or talent effect.
STOP_NAMES = {
    "B_SAY",
    "B_SAY_OVERLAY",
    "PRINT_PERCENT_BAR_SHARP",
    "PRINT_PERCENT_BAR_SHARP_NONE",
    "B_SAY_GOLD",
    "PRINTSCREEN_EXT",
}
STOP_PREFIXES = ("MEM_", "MEMINT_", "ASM_", "_@")
# Hand items the station adapters read from the world, per station.
HAND_ITEMS = {
    "marcello": {
        "stove": [16607, 16608],
        "alchemy": [16604],
        "leather": [17342],
        "saw": [16609],
        "forge": [16666, 16667],
        "anvil": [16666, 16435],
        "cool": [16612],
        "grind": [16613],
    },
    "dndr": {"alchemy": [6906], "anvil": [6913, 6912, 6767]},
}
# Catalog sources outside include/ubc/stations/<station>/ by route.
SOURCE_ROUTES = (
    (re.compile(r"(^|/)alchemy_(extra|recipe)"), "alchemy_extra"),
    (re.compile(r"(^|/)station_recipe"), "station"),
    (re.compile(r"(^|/)(recipe_|lhiver_recipe)"), "base"),
    (re.compile(r"(^|/)(anvil|ore|spirit|bow|cool|grind|tobacco)_"), None),
)

_BINARY = set(range(0, 9)) | set(range(11, 19))
_ASSIGNS = {9, 19, 20, 21, 22, 70, 71, 72, 73, 74}
_UNARY = {30, 31, 32, 33}
_PUSHES = {65, 66, 67, 68, 245}
ASSIGN_FUNC = 72
_EQUALITY = {16, 17}


class ProfileError(ValueError):
    """The reference facts do not form a consistent profile."""


def plausible_id(dat: DatFile, value: int) -> bool:
    if not 0 < value < len(dat.symbols):
        return False
    symbol = dat.symbols[value]
    return symbol.type in (INSTANCE, FUNC, PROTOTYPE) and not symbol.flags & (
        EXTERNAL | CLASSVAR
    )


def consumers(dat: DatFile, start: int, end: int) -> tuple[list, dict[int, tuple]]:
    """Map each PUSH_INT (by instruction position) to the slot consuming it."""
    code = dat.instructions(start, end)
    leaders = {start}
    for instruction in code:
        if (
            instruction.opcode in (OP_JUMP, OP_JUMPF)
            and start <= instruction.operand < end
        ):
            leaders.add(instruction.operand)
        if instruction.opcode in (OP_RET, OP_JUMP, OP_JUMPF):
            leaders.add(instruction.offset + instruction.size)
    slot_of: dict[int, tuple] = {}
    stack: list[tuple] = []

    def consume(entry: tuple, slot: tuple) -> None:
        if entry[0] == "int":
            slot_of[entry[1]] = slot

    def pop() -> tuple:
        return stack.pop() if stack else ("value", "argument")

    def drain(slot: tuple) -> None:
        for entry in stack:
            consume(entry, slot)
        stack.clear()

    symbols = dat.symbols
    for k, instruction in enumerate(code):
        opcode = instruction.opcode
        if instruction.offset in leaders:
            drain(("dangling",))
        if opcode == OP_PUSH_INT:
            stack.append(("int", k))
        elif opcode in _PUSHES:
            stack.append(("value", "variable"))
        elif opcode in _BINARY:
            right, left = pop(), pop()
            for entry, other in ((left, right), (right, left)):
                if opcode in _EQUALITY and other == (
                    "value",
                    ("return", "HLP_GETINSTANCEID"),
                ):
                    consume(entry, ("instance_id",))
                else:
                    consume(entry, ("operator", opcode))
            stack.append(("value", "expression"))
        elif opcode in _UNARY:
            consume(pop(), ("unary", opcode))
            stack.append(("value", "expression"))
        elif opcode in _ASSIGNS:
            previous = code[k - 1] if k else None
            target = (
                symbols[previous.operand].name
                if previous is not None
                and previous.opcode in SYMBOL_OPERANDS
                and 0 <= previous.operand < len(symbols)
                else "?"
            )
            pop()
            consume(pop(), ("assign", opcode, target))
        elif opcode == OP_CALL_EXTERNAL:
            external = (
                symbols[instruction.operand]
                if 0 <= instruction.operand < len(symbols)
                else None
            )
            argc = external.elements if external is not None else 0
            arguments = [pop() for _ in range(argc)][::-1]
            for position, entry in enumerate(arguments):
                consume(entry, ("external", external.name, position))
            if external is not None and external.returns:
                stack.append(("value", ("return", external.name)))
        elif opcode == OP_CALL:
            callee = dat.entry(instruction.operand)
            argc = callee.elements if callee is not None and callee.type == FUNC else 0
            name = callee.name if callee is not None else f"?{instruction.operand}"
            arguments = [pop() for _ in range(argc)][::-1]
            for position, entry in enumerate(arguments):
                consume(entry, ("call", name, position))
            if callee is not None and callee.returns:
                stack.append(("value", ("return", name)))
        elif opcode == OP_JUMPF:
            consume(pop(), ("condition",))
        elif opcode == OP_RET:
            drain(("return",))
    drain(("dangling",))
    return code, slot_of


def merged(ranges) -> list[tuple[int, int]]:
    """Joins overlapping and adjacent [begin, end) reads; verifiers hash long
    functions in several blocks."""
    out: list[list[int]] = []
    for begin, end in sorted(ranges):
        if out and begin <= out[-1][1]:
            out[-1][1] = max(out[-1][1], end)
        else:
            out.append([begin, end])
    return [(begin, end) for begin, end in out]


def campaign_source(campaign: str, source: str) -> bool:
    return source.rsplit("/", 1)[-1].startswith("dndr_") == (campaign == "dndr")


def block_key(recipe: dict) -> tuple[int, int] | None:
    if ROUTES[recipe["route"]] not in BLOCK_ROUTES:
        return None
    return recipe["route"], recipe["station"]


def code_symbols(dat: DatFile) -> list[Symbol]:
    return [dat.entries[start][0] for start in dat.starts]


def id_slots(dat: DatFile) -> set[tuple]:
    """Slots whose PUSH_INT values are symbol indices throughout the script."""
    values: dict[tuple, list[int]] = defaultdict(list)
    for symbol in code_symbols(dat):
        start, end = dat.extent(symbol)
        try:
            code, slot_of = consumers(dat, start, end)
        except ValueError:
            continue
        for k, slot in slot_of.items():
            values[slot].append(code[k].operand)
    slots = set()
    for slot, seen in values.items():
        if slot[0] in ("dangling", "condition", "return", "unary"):
            continue
        if slot == ("instance_id",) or (slot[0] == "assign" and slot[1] == ASSIGN_FUNC):
            slots.add(slot)
        elif all(plausible_id(dat, value) for value in seen) and max(seen) >= 1000:
            slots.add(slot)
    return slots


@dataclass
class Function:
    symbol: Symbol
    length: int
    symbolic: list[int]
    operands: set[int]
    callees: list[int]
    anonymous: dict[int, list[int]]  # text index -> relative offsets


@dataclass
class Group:
    recipe: int
    station: int
    functions: set[int] = field(default_factory=set)
    data: set[int] = field(default_factory=set)
    # Named string constants whose text the verifier reads (text records).
    texts: set[int] = field(default_factory=set)


@dataclass
class Profile:
    campaign: str
    dat: DatFile
    names: list[tuple[str, int]]
    functions: list[dict]
    data: list[dict]
    groups: list[dict]
    stations: list[dict]
    anonymous_texts: list[dict]
    summary: dict


class Builder:
    def __init__(
        self,
        campaign: str,
        dat: DatFile,
        recipes: list[dict],
        literals: tuple[list[FunctionLiteral], list[BindingLiteral]],
    ):
        self.campaign, self.dat, self.recipes = campaign, dat, recipes
        self.function_literals, self.binding_literals = literals
        self.slots = id_slots(dat)
        self.cache: dict[int, Function] = {}

    # --- script facts ---------------------------------------------------
    def function(self, index: int) -> Function:
        if index in self.cache:
            return self.cache[index]
        dat = self.dat
        symbol = dat.symbols[index]
        extent = dat.extent(symbol)
        if extent is None or dat.entry(extent[0]) is not symbol:
            raise ProfileError(f"{symbol.name}: no unique code entry")
        start, end = extent
        if end - start > fp.MAX_FUNCTION_LENGTH:
            raise ProfileError(
                f"{symbol.name}: {end - start} bytes exceed the UBCF1 limit "
                f"of {fp.MAX_FUNCTION_LENGTH} bytes"
            )
        code, slot_of = consumers(dat, start, end)
        symbolic, operands, callees, anonymous = [], set(), [], defaultdict(list)
        for k, instruction in enumerate(code):
            if instruction.opcode == OP_PUSH_INT:
                if slot_of.get(k) in self.slots and plausible_id(
                    dat, instruction.operand
                ):
                    symbolic.append(instruction.offset - start)
                    operands.add(instruction.operand)
            elif instruction.opcode == OP_CALL:
                callee = dat.entry(instruction.operand)
                if callee is None or callee.anonymous or not callee.name:
                    raise ProfileError(f"{symbol.name}: call without unique callee")
                callees.append(callee.index)
            elif instruction.opcode in SYMBOL_OPERANDS:
                target = dat.symbols[instruction.operand]
                if target.anonymous:
                    anonymous[target.index].append(instruction.offset - start)
                else:
                    operands.add(target.index)
        result = Function(
            symbol, end - start, symbolic, operands, callees, dict(anonymous)
        )
        self.cache[index] = result
        return result

    def stopped(self, index: int) -> bool:
        name = self.dat.symbols[index].name
        return name in STOP_NAMES or name.startswith(STOP_PREFIXES)

    def npc_constructor(self, index: int) -> bool:
        symbol = self.dat.symbols[index]
        return symbol.type == INSTANCE and self.dat.class_of(index) == "C_NPC"

    def closure(self, roots: set[int]) -> tuple[set[int], set[int]]:
        seen, pending, stops = set(roots), list(roots), set()
        while pending:
            current = pending.pop()
            if self.npc_constructor(current):
                continue
            for callee in self.function(current).callees:
                if self.stopped(callee) and callee not in roots:
                    stops.add(callee)
                elif callee not in seen:
                    seen.add(callee)
                    pending.append(callee)
        return seen, stops

    def is_item(self, index: int) -> bool:
        symbol = self.dat.symbols[index]
        return (
            symbol.type in (INSTANCE, PROTOTYPE)
            and self.dat.class_of(index) == "C_ITEM"
        )

    # --- catalog facts --------------------------------------------------
    def function_matches(self, literal: FunctionLiteral) -> bool:
        dat = self.dat
        if (
            literal.symbol >= len(dat.symbols)
            or dat.symbols[literal.symbol].name != literal.name
        ):
            return False
        body = dat.code[literal.start : literal.start + literal.length]
        return hashlib.sha256(body).hexdigest() == literal.raw_sha256

    def binding_matches(self, literal: BindingLiteral) -> bool:
        if literal.symbol >= len(self.dat.symbols):
            return False
        symbol = self.dat.symbols[literal.symbol]
        same_name = symbol.name == literal.name or (
            symbol.anonymous and literal.name.startswith("\xff")
        )
        return same_name and (symbol.type, symbol.flags, symbol.elements) == (
            literal.type,
            literal.flags,
            literal.elements,
        )

    def matching_literals(self) -> tuple[list[FunctionLiteral], list[BindingLiteral]]:
        """Literals of this campaign; each must match the reference script.
        Literals of the other campaign that match by chance are counted in
        self.foreign_matches and ignored."""
        functions, bindings, unmatched = [], [], []
        self.foreign_matches = 0
        for literals, matches, out in (
            (self.function_literals, self.function_matches, functions),
            (self.binding_literals, self.binding_matches, bindings),
        ):
            for literal in literals:
                matched = matches(literal)
                if not campaign_source(self.campaign, literal.source):
                    self.foreign_matches += matched
                elif matched:
                    out.append(literal)
                else:
                    unmatched.append(f"{literal.source}: {literal.name!r}")
        if unmatched:
            raise ProfileError(
                f"{len(unmatched)} catalog literals do not match the reference "
                f"script: {unmatched[:3]}"
            )
        return functions, bindings

    def source_stations(self, source: str) -> set[int]:
        """Stations with recipes of this campaign that a catalog source serves."""
        parts = source.split("/")
        by_route: dict[str, set[int]] = defaultdict(set)
        for recipe in self.recipes:
            by_route[ROUTES[recipe["route"]]].add(recipe["station"])
        present = {recipe["station"] for recipe in self.recipes}
        if (
            len(parts) > 3
            and parts[1] == "ubc"
            and parts[2] == "stations"
            and parts[3] in STATIONS
        ):
            directory = parts[3]
            stem = parts[-1]
            if directory == "alchemy" and "alchemy_extra" in stem:
                return by_route["alchemy_extra"]
            return {STATIONS.index(directory)} & present
        if len(parts) > 3 and parts[2] == "stations" and parts[3] in ROUTES:
            return by_route[parts[3]]
        stem = parts[-1]
        for pattern, route in SOURCE_ROUTES:
            match = pattern.search(stem)
            if match:
                return by_route[route or match[2]]
        return set()

    # --- assembly -------------------------------------------------------
    def build(self) -> Profile:
        dat = self.dat
        recipes = self.recipes
        if not recipes or not all(recipe["matches"] for recipe in recipes):
            raise ProfileError("the plugin verifiers do not match the reference script")
        outside = [f"{r['id']:#x}" for r in recipes if r.get("outside", 0)]
        if outside:
            raise ProfileError(
                f"verifiers read outside the parser image: {outside[:5]}"
            )
        block_owned: dict[tuple[int, int], set[int]] = defaultdict(set)
        for recipe in recipes:
            key = block_key(recipe)
            if key is not None:
                block_owned[key] |= {
                    index for role, index in recipe["own"] if role in BLOCK_ROLES
                }
        groups: list[Group] = []
        roots_by_group: list[set[int]] = []
        stops_by_group: list[set[int]] = []
        seen_functions: set[int] = set()
        seen_data: set[int] = set()
        recorded_texts: dict[int, set[int]] = {}
        exempt_by_group: list[set[int]] = []
        for recipe in recipes:
            hashed = set()
            bound = set()
            read_texts = set()
            reads = merged(recipe["code"])
            for slot in recipe["slots"]:
                if slot >= len(dat.symbols):
                    raise ProfileError(
                        f"recipe {recipe['id']:#x}: slot {slot} outside the script"
                    )
                extent = dat.extent(dat.symbols[slot])
                if extent and any(a < extent[1] and extent[0] < b for a, b in reads):
                    hashed.add(slot)
                else:
                    bound.add(slot)
            for index in recipe.get("strings", ()):
                if not 0 <= index < len(dat.symbols):
                    raise ProfileError(
                        f"recipe {recipe['id']:#x}: text {index} outside the script"
                    )
                bound.add(index)
                if not dat.symbols[index].anonymous:
                    self.text_record(index)
                    read_texts.add(index)
            texts = {slot for slot in bound if dat.symbols[slot].anonymous}
            bound -= texts
            for text in texts:
                recorded_texts.setdefault(text, set()).add(recipe["station"])
            own = {entry[1] for entry in recipe["own"]}
            key = block_key(recipe)
            others = block_owned[key] - own if key is not None else set()
            roots = hashed - others
            bound -= others
            exempt_by_group.append(others)
            functions, stops = self.closure(roots)
            group = Group(
                recipe["id"], recipe["station"], functions, set(bound), read_texts
            )
            items = (
                set(recipe["targets"]) | set(recipe["costs"]) | set(recipe["outputs"])
            )
            items |= set(recipe.get("ingredients", ())) | {
                p for p in recipe.get("products", ()) if p > 0
            }
            station_name = STATIONS[recipe["station"]]
            items |= set(HAND_ITEMS.get(self.campaign, {}).get(station_name, ()))
            for item in items:
                if not 0 < item < len(dat.symbols) or not self.is_item(item):
                    raise ProfileError(f"recipe {recipe['id']:#x}: {item} is no item")
            group.data |= items | stops
            groups.append(group)
            roots_by_group.append(roots)
            stops_by_group.append(stops)
            seen_functions |= functions
            seen_data |= group.data

        literal_functions, literal_bindings = self.matching_literals()
        station_functions: dict[int, set[int]] = defaultdict(set)
        station_data: dict[int, set[int]] = defaultdict(set)
        anonymous_refs: list[tuple[int, int, int]] = []  # text, function, offset
        text_functions: set[int] = set()
        unplaced: set[str] = set()
        for literal in literal_functions:
            if literal.symbol in seen_functions:
                continue
            stations = self.source_stations(literal.source)
            if not stations:
                unplaced.add(f"{literal.source}: {literal.name}")
            for station in stations:
                station_functions[station].add(literal.symbol)
        anonymous_bindings = sorted(
            {
                b.symbol: b for b in literal_bindings if dat.symbols[b.symbol].anonymous
            }.values(),
            key=lambda b: b.symbol,
        )
        for literal in literal_bindings:
            if dat.symbols[literal.symbol].anonymous or literal.symbol in seen_data:
                continue
            stations = self.source_stations(literal.source)
            if not stations:
                unplaced.add(f"{literal.source}: {literal.name}")
            for station in stations:
                station_data[station].add(literal.symbol)
        text_stations: dict[int, set[int]] = {
            t: set(s) for t, s in recorded_texts.items()
        }
        for literal in anonymous_bindings:
            text_stations.setdefault(literal.symbol, set()).update(
                self.source_stations(literal.source)
            )
        for text, stations in sorted(text_stations.items()):
            if not stations:
                unplaced.add(f"anonymous text {text}")
        if unplaced:
            raise ProfileError(
                f"catalog literals outside every station: {sorted(unplaced)[:5]}"
            )
        users = self.text_users(set(text_stations))
        for text in sorted(text_stations):
            if text not in users:
                raise ProfileError(f"anonymous text {text} has no user")
            function, offset = users[text]
            anonymous_refs.append((text, function, offset))
            text_functions.add(function)
            for station in text_stations[text]:
                station_functions[station].add(function)

        by_station: dict[int, list[Group]] = defaultdict(list)
        for group in groups:
            by_station[group.station].append(group)
        shares: dict[int, tuple[set[int], set[int]]] = {}
        share_texts: dict[int, set[int]] = {}
        for station, members in sorted(by_station.items()):
            share_texts[station] = (
                set.intersection(*(g.texts for g in members))
                if len(members) > 1
                else set()
            )
            common_f = (
                set.intersection(*(g.functions for g in members))
                if len(members) > 1
                else set()
            )
            common_d = (
                set.intersection(*(g.data for g in members))
                if len(members) > 1
                else set()
            )
            extra_f, _ = self.closure(station_functions.get(station, set()))
            shares[station] = (
                common_f | extra_f,
                common_d | station_data.get(station, set()),
            )
            for group in members:
                group.functions |= shares[station][0]
                group.data |= shares[station][1]
        # Records: functions first, then every data symbol they name.
        record_functions = set()
        for group in groups:
            record_functions |= group.functions
        record_functions |= {literal.symbol for literal in literal_functions}
        record_functions |= text_functions
        for station, extra in station_functions.items():
            record_functions |= self.closure(extra)[0]
        for group in groups:
            for index in sorted(group.functions):
                facts = self.function(index)
                for operand in facts.operands:
                    if operand not in group.functions:
                        group.data.add(operand)
                for callee in facts.callees:
                    if callee not in group.functions:
                        group.data.add(callee)
                if 0 <= facts.symbol.parent:
                    group.data.add(facts.symbol.parent)
        for group in groups:
            group.data = self.with_parents(group.data)
        for station, (functions, data) in shares.items():
            shares[station] = (
                functions,
                self.with_parents(data | self.named_by(functions)),
            )
        self.check_code_reads(groups, shares, exempt_by_group)
        # A group keeps only its recipe share; relocate() adds the station share.
        for group in groups:
            share_functions, share_data = shares[group.station]
            group.functions -= share_functions
            group.data -= share_data
            group.texts -= share_texts[group.station]
        record_data = set()
        for group in groups:
            record_data |= group.data
        for _, data in shares.values():
            record_data |= data
        record_data |= {
            b.symbol for b in literal_bindings if not dat.symbols[b.symbol].anonymous
        }
        record_data = self.with_parents(record_data)
        names = set(record_functions) | set(record_data)
        for index in record_functions:
            facts = self.function(index)
            names |= facts.operands | set(facts.callees)
            if facts.symbol.parent >= 0:
                names.add(facts.symbol.parent)
        names |= self.with_parents(names) - names
        return self.assemble(
            groups,
            {s: (f, d, share_texts[s]) for s, (f, d) in shares.items()},
            record_functions,
            record_data,
            names,
            literal_functions,
            literal_bindings,
            anonymous_refs,
            text_functions,
        )

    def check_code_reads(self, groups, shares, exempt_by_group) -> None:
        """Every code byte a verifier read lies in a recorded function."""
        gaps = []
        for group, recipe, exempt in zip(groups, self.recipes, exempt_by_group):
            covered = merged(
                self.dat.extent(self.dat.symbols[index])
                for index in group.functions | shares[group.station][0] | exempt
            )
            for begin, end in merged(recipe["code"]):
                if not any(a <= begin and end <= b for a, b in covered):
                    gaps.append(f"{recipe['id']:#x}: {begin}..{end}")
        if gaps:
            raise ProfileError(f"verifier code reads outside every group: {gaps[:5]}")

    def scalar_bits(self, symbol: Symbol) -> int:
        """Value of a constant INT, or the raw 32-bit pattern of a FLOAT."""
        value = symbol.content[0]
        if symbol.type == INT:
            return value
        if math.isnan(value):
            raise ProfileError(f"{symbol.name}: NaN constant")
        return struct.unpack("<i", struct.pack("<f", value))[0]

    def text_record(self, index: int) -> tuple[int, str]:
        """Length and SHA-256 of a named string constant a verifier reads."""
        symbol = self.dat.symbols[index]
        if (
            symbol.type != STRING
            or symbol.flags != 1
            or symbol.elements != 1
            or symbol.parent >= 0
            or not isinstance(symbol.content, list)
            or len(symbol.content) != 1
        ):
            raise ProfileError(f"{symbol.name}: read text is no plain string constant")
        text = symbol.content[0]
        if b"\xff" in text or len(text) > MAX_TEXT_LENGTH:
            raise ProfileError(f"{symbol.name}: read text cannot be bound")
        return len(text), hashlib.sha256(text).hexdigest()

    def named_by(self, functions: set[int]) -> set[int]:
        out = set()
        for index in functions:
            facts = self.function(index)
            out |= {
                x for x in facts.operands | set(facts.callees) if x not in functions
            }
            if facts.symbol.parent >= 0:
                out.add(facts.symbol.parent)
        return out

    def with_parents(self, indices: set[int]) -> set[int]:
        out = set(indices)
        pending = list(indices)
        while pending:
            parent = self.dat.symbols[pending.pop()].parent
            if 0 <= parent < len(self.dat.symbols) and parent not in out:
                out.add(parent)
                pending.append(parent)
        return out

    def text_users(self, texts: set[int]) -> dict[int, tuple[int, int]]:
        """First function (by code position) that names each anonymous text."""
        found: dict[int, tuple[int, int]] = {}
        for symbol in code_symbols(self.dat):
            if len(found) == len(texts):
                break
            start, end = self.dat.extent(symbol)
            try:
                code = self.dat.instructions(start, end)
            except ValueError:
                continue
            for instruction in code:
                if (
                    instruction.opcode in SYMBOL_OPERANDS
                    and instruction.operand in texts
                    and instruction.operand not in found
                ):
                    found[instruction.operand] = (
                        symbol.index,
                        instruction.offset - start,
                    )
        return found

    def assemble(
        self,
        groups,
        shares,
        record_functions,
        record_data,
        names,
        literal_functions,
        literal_bindings,
        anonymous_refs,
        text_functions,
    ) -> Profile:
        dat = self.dat
        for index in names:
            symbol = dat.symbols[index]
            if (
                not symbol.name
                or symbol.anonymous
                or len(symbol.name.encode("latin-1")) > 63
            ):
                raise ProfileError(f"symbol {index} has no usable name")
        counts = Counter(symbol.name for symbol in dat.symbols if symbol.name)
        duplicate = sorted(
            dat.symbols[i].name for i in names if counts[dat.symbols[i].name] > 1
        )
        if duplicate:
            raise ProfileError(f"duplicate names in the reference: {duplicate[:5]}")
        ordered = sorted(names, key=lambda i: dat.symbols[i].name)
        name_id = {index: k for k, index in enumerate(ordered)}
        literal_length = {}
        for literal in literal_functions:
            literal_length[literal.symbol] = literal.length
        functions = []
        for index in sorted(record_functions, key=lambda i: dat.symbols[i].name):
            facts = self.function(index)
            symbol = facts.symbol
            length = literal_length.get(index, facts.length)
            if length != facts.length:
                raise ProfileError(
                    f"{symbol.name}: catalog length {length} != {facts.length}"
                )
            header = fp.Header(
                symbol.name,
                symbol.type,
                symbol.flags,
                symbol.elements,
                symbol.offset if symbol.type == FUNC else 0,
                dat.parent_name(symbol),
            )
            result = fp.fingerprint(
                dat.code,
                symbol.address,
                length,
                header,
                SymbolNames(dat),
                EntryNames(dat),
                facts.symbolic,
                False,
            )
            functions.append(
                {
                    "index": index,
                    "name": name_id[index],
                    "length": length,
                    "canonical": result.canonical,
                    "start": symbol.address,
                    "raw": result.raw,
                    "type": symbol.type,
                    "flags": symbol.flags,
                    "argc": symbol.elements,
                    "return_type": header.return_type,
                    "parent": name_id[symbol.parent] if symbol.parent >= 0 else None,
                    "symbolic": facts.symbolic,
                    "ignore_text": False,
                }
            )
        function_id = {record["index"]: k for k, record in enumerate(functions)}
        binding_checks: dict[int, dict] = {}
        for literal in literal_bindings:
            entry = binding_checks.setdefault(literal.symbol, {})
            if literal.check_value:
                entry["value"] = literal.value
            if literal.check_offset:
                entry["offset"] = literal.offset
        # A read text gets its own record next to the plain one, so that only
        # the recipes whose verifier reads the text depend on it.
        text_checks = set().union(*(g.texts for g in groups))
        text_checks |= set().union(*(texts for _, _, texts in shares.values()))
        keys = [(index, False) for index in record_data]
        keys += [(index, True) for index in text_checks]
        data = []
        for index, read_text in sorted(
            keys, key=lambda k: (dat.symbols[k[0]].name, k[1])
        ):
            symbol = dat.symbols[index]
            checks = binding_checks.get(index, {})
            check_offset = bool(symbol.flags & CLASSVAR) or "offset" in checks
            scalar = (
                symbol.type in (INT, FLOAT)
                and symbol.flags == 1
                and symbol.elements == 1
                and isinstance(symbol.content, list)
            )
            check_value = scalar or "value" in checks
            value = self.scalar_bits(symbol) if scalar else checks.get("value", 0)
            text = self.text_record(index) if read_text else None
            if "value" in checks and checks["value"] != value:
                raise ProfileError(
                    f"{symbol.name}: catalog value differs from the script"
                )
            data.append(
                {
                    "index": index,
                    "name": name_id[index],
                    "type": symbol.type,
                    "flags": symbol.flags,
                    "elements": symbol.elements,
                    "parent": name_id[symbol.parent] if symbol.parent >= 0 else None,
                    "check_offset": check_offset,
                    "offset": symbol.offset if check_offset else 0,
                    "check_value": check_value,
                    "value": value,
                    "text": text,
                }
            )
        data_id = {
            record["index"]: k for k, record in enumerate(data) if not record["text"]
        }
        text_id = {
            record["index"]: k for k, record in enumerate(data) if record["text"]
        }
        station_out = []
        for station, (functions_, data_, texts_) in sorted(shares.items()):
            station_out.append(
                {
                    "station": station,
                    "functions": sorted(function_id[i] for i in functions_),
                    "data": sorted(
                        [data_id[i] for i in data_] + [text_id[i] for i in texts_]
                    ),
                }
            )
        share_id = {entry["station"]: k for k, entry in enumerate(station_out)}
        group_out = []
        for group in groups:
            group_out.append(
                {
                    "recipe": group.recipe,
                    "station": share_id[group.station],
                    "functions": sorted(function_id[i] for i in group.functions),
                    "data": sorted(
                        [data_id[i] for i in group.data]
                        + [text_id[i] for i in group.texts]
                    ),
                }
            )
        texts_out = [
            {
                "reference_index": text,
                "function": function_id[function],
                "offset": offset,
            }
            for text, function, offset in anonymous_refs
        ]
        texts_seen = {t for t, _, _ in anonymous_refs}
        missing_functions = sorted(
            {f.name for f in literal_functions if f.symbol not in function_id}
        )
        missing_bindings = sorted(
            {
                b.name
                for b in literal_bindings
                if b.symbol not in data_id and b.symbol not in texts_seen
            }
        )
        if missing_functions or missing_bindings:
            raise ProfileError(
                f"catalog literals without record: {missing_functions[:3]} "
                f"{missing_bindings[:3]}"
            )
        summary = {
            "names": len(ordered),
            "functions": len(functions),
            "data": len(data),
            "groups": len(group_out),
            "stations": len(station_out),
            "texts": len(texts_out),
            "symbolic_ints": sum(len(r["symbolic"]) for r in functions),
            "function_bytes": sum(r["length"] for r in functions),
            "catalog_functions": len({f.symbol for f in literal_functions}),
            "catalog_bindings": len({b.symbol for b in literal_bindings}),
            "text_checks": sum(1 for r in data if r["text"]),
            "float_checks": sum(
                1 for r in data if r["check_value"] and r["type"] == FLOAT
            ),
            "foreign_matches": self.foreign_matches,
        }
        return Profile(
            self.campaign,
            dat,
            [(dat.symbols[i].name, i) for i in ordered],
            functions,
            data,
            group_out,
            station_out,
            texts_out,
            summary,
        )


class SymbolNames:
    """Operand view for fingerprint.py: names, or the text of anonymous strings.

    Texts are hashed as the exact DAT bytes. A bound text containing 0xff is
    refused: the in-memory form of that byte is not verified, and a guessed
    translation could make the digest differ from the loaded parser.
    """

    def __init__(self, dat: DatFile, bind_text: bool = True):
        self.dat = dat
        self.bind_text = bind_text

    def get(self, index: int, default=None):
        if not 0 <= index < len(self.dat.symbols):
            return default
        symbol = self.dat.symbols[index]
        if symbol.anonymous:
            if (
                symbol.type != STRING
                or symbol.flags != 1
                or symbol.elements != 1
                or symbol.parent >= 0
                or not isinstance(symbol.content, list)
            ):
                return default
            text = symbol.content[0]
            if self.bind_text and b"\xff" in text:
                raise ProfileError(f"bound text {index} contains 0xff")
            return fp.Text(text)
        return symbol.name or default


class EntryNames:
    def __init__(self, dat: DatFile):
        self.dat = dat

    def get(self, address: int, default=None):
        symbol = self.dat.entry(address)
        return (
            symbol.name
            if symbol is not None and symbol.name and not symbol.anonymous
            else default
        )


# --- header rendering ---------------------------------------------------
def c_string(text: str) -> str:
    out = []
    for byte in text.encode("latin-1"):
        if byte in (0x22, 0x5C):
            out.append("\\" + chr(byte))
        elif 0x20 <= byte < 0x7F:
            out.append(chr(byte))
        else:
            out.append(f"\\{byte:03o}")
    return '"' + "".join(out) + '"'


def render(profile: Profile, config: dict) -> str:
    def name(value):
        return "none" if value is None else str(value)

    def flag(value: bool) -> str:
        return "true" if value else "false"

    lines = [
        "// SPDX-License-Identifier: MIT",
        "// Generated by tools/profile/generate-script-profile-WRITES.py. Do not edit.",
        f"//   mise run profile:generate-WRITES -- --campaign {profile.campaign} --dat <GOTHIC.DAT> --apply",
        f"// Reference: {config['title']}, GOTHIC.DAT {profile.dat.size} bytes.",
        "// Names, signatures, lengths, offsets, constant values and digests only;",
        "// no bytecode and no texts. Data for ubc/engine/script_relocation.hpp.",
        "#pragma once",
        "",
        '#include "ubc/catalogs/lhiver_recipe_profile.hpp"',
        '#include "ubc/engine/script_relocation.hpp"',
        "#include <array>",
        "#include <cstdint>",
        "",
        f"namespace ubc::script_profiles::{config['namespace']} {{",
        "namespace sr = script_relocation;",
        "template <std::size_t N> constexpr sr::Digest h(const char (&text)[N]) {",
        "  return lhiver::digest_literal(text);",
        "}",
        "inline constexpr std::uint32_t none = sr::no_name;",
        f"inline constexpr std::uint32_t dat_size = {profile.dat.size};",
        f'inline constexpr auto dat_sha256 = h("{profile.dat.sha256}");',
        f"inline constexpr std::uint32_t symbol_count = {len(profile.dat.symbols)};",
        "",
    ]

    def array(
        kind: str, label: str, items: list[str], count: int | None = None
    ) -> None:
        count = len(items) if count is None else count
        lines.append(f"inline constexpr std::array<{kind}, {count}> {label}{{{{")
        lines.extend(f"    {item}," for item in items)
        lines.append("}};")
        lines.append("")

    array(
        "sr::Name",
        "names",
        [f"{{{c_string(text)}, {index}}}" for text, index in profile.names],
    )
    permutation = sorted(range(len(profile.names)), key=lambda k: profile.names[k][1])
    array("std::uint32_t", "by_reference", _packed(permutation), len(permutation))
    symbolic: list[int] = []
    starts = []
    for record in profile.functions:
        starts.append(len(symbolic))
        symbolic.extend(record["symbolic"])
    array("std::uint32_t", "symbolic_ints", _packed(symbolic), len(symbolic))
    lines += ["inline constexpr const std::uint32_t* si = symbolic_ints.data();", ""]
    items = []
    for record, start in zip(profile.functions, starts):
        pointer = f"si + {start}" if record["symbolic"] else "nullptr"
        items.append(
            f'{{{record["name"]}, {record["length"]}, h("{record["canonical"]}"), '
            f'{record["start"]}, h("{record["raw"]}"), {record["type"]}, '
            f"{record['flags']}, {record['argc']}, {record['return_type']}, {name(record['parent'])}, "
            f"{pointer}, {len(record['symbolic'])}, {flag(record['ignore_text'])}}}"
        )
    array("sr::FunctionRecord", "functions", items)

    def data_item(r: dict) -> str:
        text = f', true, {r["text"][0]}, h("{r["text"][1]}")' if r["text"] else ""
        return (
            f"{{{r['name']}, {r['type']}, {r['flags']}, {r['elements']}, {name(r['parent'])}, "
            f"{flag(r['check_offset'])}, {r['offset']}, {flag(r['check_value'])}, {r['value']}"
            f"{text}}}"
        )

    array("sr::DataRecord", "data", [data_item(r) for r in profile.data])
    function_lists: list[int] = []
    data_lists: list[int] = []

    def lists(entry: dict) -> str:
        f_start, d_start = len(function_lists), len(data_lists)
        function_lists.extend(entry["functions"])
        data_lists.extend(entry["data"])
        f_ptr = f"gf + {f_start}" if entry["functions"] else "nullptr"
        d_ptr = f"gd + {d_start}" if entry["data"] else "nullptr"
        return f"{f_ptr}, {len(entry['functions'])}, {d_ptr}, {len(entry['data'])}"

    station_items = [f"{{{s['station']}, {lists(s)}}}" for s in profile.stations]
    group_items = [
        f"{{{g['recipe']:#x}, {lists(g)}, {g['station']}}}" for g in profile.groups
    ]
    array(
        "std::uint32_t", "group_functions", _packed(function_lists), len(function_lists)
    )
    array("std::uint32_t", "group_data", _packed(data_lists), len(data_lists))
    lines += [
        "inline constexpr const std::uint32_t* gf = group_functions.data();",
        "inline constexpr const std::uint32_t* gd = group_data.data();",
        "",
    ]
    array("sr::StationShare", "stations", station_items)
    array("sr::RecipeGroup", "groups", group_items)
    array(
        "sr::AnonymousText",
        "anonymous_texts",
        [
            f"{{{t['reference_index']}, {t['function']}, {t['offset']}}}"
            for t in profile.anonymous_texts
        ],
    )
    lines += [
        "inline constexpr sr::ReferenceProfile profile{",
        f"    {config['parser']:#010x}, names.data(), names.size(), functions.data(), functions.size(),",
        "    data.data(), data.size(), groups.data(), groups.size(), by_reference.data(),",
        "    stations.data(), stations.size(), anonymous_texts.data(), anonymous_texts.size()};",
        "",
        f"}} // namespace ubc::script_profiles::{config['namespace']}",
        "",
    ]
    return "\n".join(lines)


def _packed(values: list[int], width: int = 96) -> list[str]:
    if not values:
        return []
    rows, row = [], ""
    for value in values:
        token = f"{value}, "
        if row and len(row) + len(token) > width:
            rows.append(row.rstrip(", "))
            row = ""
        row += token
    rows.append(row.rstrip(", "))
    return rows
