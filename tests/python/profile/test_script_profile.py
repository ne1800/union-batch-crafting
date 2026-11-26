"""Profile generator over a synthetic script and the committed profile headers."""

from pathlib import Path
import re
import struct
import sys
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tools/profile"))
import catalog_literals  # noqa: E402
import fingerprint  # noqa: E402
import script_profile  # noqa: E402
from catalog_literals import BindingLiteral, FunctionLiteral  # noqa: E402
from datfile import CLASS, FUNC, INSTANCE, INT, STRING, DatFile  # noqa: E402

GENERATED = ROOT / "include/ubc/catalogs/generated"
HEADERS = ("marcello_204_fingerprints.hpp", "dndr_26_fingerprints.hpp")
HOOK_LIMIT = 500 * 1024


def word(opcode, value):
    return bytes((opcode,)) + struct.pack("<i", value)


class Script:
    """Writes a minimal compiled script in the GOTHIC.DAT layout."""

    def __init__(self):
        self.symbols = []
        self.code = bytearray()

    def add(self, name, kind, flags=0, elements=1, content=None, parent=-1, offset=0):
        self.symbols.append([name, kind, flags, elements, content, parent, offset])
        return len(self.symbols) - 1

    def function(self, name, body, kind=FUNC, parent=-1, argc=0, flags=1):
        index = self.add(name, kind, flags, argc, len(self.code), parent)
        self.code += body
        return index

    def bytes(self):
        out = bytearray(b"\x32" + struct.pack("<I", len(self.symbols)))
        out += struct.pack(f"<{len(self.symbols)}I", *range(len(self.symbols)))
        for name, kind, flags, elements, content, parent, offset in self.symbols:
            out += struct.pack("<I", 1) + name.encode("latin-1") + b"\n"
            out += struct.pack(
                "<iIiiiii", offset, elements | kind << 12 | flags << 16, 0, 0, 0, 0, 0
            )
            if kind == INT and not flags & 4:
                out += struct.pack(f"<{elements}i", *content)
            elif kind == STRING:
                out += b"".join(text + b"\n" for text in content)
            elif kind in (CLASS, FUNC, 6, INSTANCE):
                out += struct.pack("<i", content or 0)
            out += struct.pack("<i", parent)
        return bytes(out + struct.pack("<I", len(self.code)) + self.code)


def synthetic(text_bytes=b"Fertig gut"):
    s = Script()
    s.add("C_ITEM", CLASS, 0, 2, 0)
    s.add("C_ITEM.AMOUNT", INT, 4, 1, None, 0, 0x14)
    s.add("CREATEINVITEMS", FUNC, 8 | 1, 3, 0)
    s.add("UBC_AMOUNT", INT, 1, 1, [3])
    s.add("SELF", INSTANCE, 0, 1, 0)
    s.add("UBC_K\xe4SE", INT, 0, 1, [0])
    text = s.add("\xff10000", STRING, 1, 1, [text_bytes])
    for k in range(1000):
        s.add(f"UBC_FILLER_{k:04}", INT, 0, 1, [0])
    item_body = word(64, 2) + word(65, 1) + bytes((9, 60))
    item = s.function("ITMI_SAMPLE", item_body, INSTANCE, parent=0)
    other = s.function("ITMI_OTHER", item_body, INSTANCE, parent=0)
    say = s.function("B_SAY", word(65, text) + bytes((60,)))
    helper = s.function(
        "UBC_HELPER", word(64, 1) + word(65, 3) + bytes((16, 60)), flags=3
    )
    recipe = bytearray()
    for target in (item, other, item):
        recipe += word(67, 4) + word(64, target) + word(65, 3) + word(62, 2)
    recipe += (
        word(61, s.symbols[say][4]) + word(61, s.symbols[helper][4]) + bytes((60,))
    )
    main = s.function("UBC_RECIPE", bytes(recipe))
    return s, {
        "item": item,
        "other": other,
        "say": say,
        "helper": helper,
        "recipe": main,
        "text": text,
    }


def recorded(dat, ids):
    recipe = dat.symbols[ids["recipe"]]
    start, end = dat.extent(recipe)
    return [
        {
            "id": 0x2A,
            "station": 0,
            "route": 0,
            "targets": [ids["item"]],
            "costs": [],
            "outputs": [ids["item"]],
            "own": [],
            "matches": True,
            "outside": 0,
            "slots": [ids["recipe"], 3, 5],
            "code": [[start, end]],
        }
    ]


CONFIG = {"title": "synthetic", "namespace": "synthetic", "parser": 0x00AB40C0}


class SyntheticProfileTest(unittest.TestCase):
    def setUp(self):
        self.script, self.ids = synthetic()
        self.data = self.script.bytes()
        self.dat = DatFile.parse(self.data)

    def say_literals(self):
        import hashlib

        say = self.dat.symbols[self.ids["say"]]
        start, end = self.dat.extent(say)
        source = "include/ubc/stations/stove/x.hpp"
        digest = hashlib.sha256(self.dat.code[start:end]).hexdigest()
        literal = FunctionLiteral(
            "B_SAY", say.index, start, end - start, digest, 0, FUNC, 1, 0, -1, source
        )
        text = BindingLiteral(
            "\xff1", self.ids["text"], STRING, 1, 1, -1, False, 0, False, 0, source
        )
        return [literal], [text]

    def build(self, literals=([], [])):
        builder = script_profile.Builder(
            "synthetic", self.dat, recorded(self.dat, self.ids), literals
        )
        return builder.build()

    def test_reader_keeps_raw_names_and_texts(self):
        self.assertIn("UBC_K\xe4SE", self.dat.by_name)
        self.assertEqual(self.dat.symbols[self.ids["text"]].content, [b"Fertig gut"])
        self.assertEqual(self.dat.extent(self.dat.symbols[self.ids["say"]]), (24, 30))
        self.assertIsNone(self.dat.extent(self.dat.symbols[2]))

    def test_symbol_slots_come_from_the_whole_script(self):
        slots = script_profile.id_slots(self.dat)
        self.assertIn(("external", "CREATEINVITEMS", 1), slots)
        self.assertNotIn(("external", "CREATEINVITEMS", 2), slots)
        profile = self.build()
        record = next(r for r in profile.functions if r["index"] == self.ids["recipe"])
        self.assertEqual(record["symbolic"], [5, 25, 45])
        self.assertTrue(record["ignore_text"])

    def test_group_hull_records_and_digests(self):
        profile = self.build()
        names = [name for name, _ in profile.names]
        self.assertEqual(names, sorted(names))
        self.assertIn("UBC_K\xe4SE", names)
        functions = {profile.names[r["name"]][0] for r in profile.functions}
        self.assertEqual(functions, {"UBC_RECIPE", "UBC_HELPER"})
        group = profile.groups[0]
        data = {profile.names[profile.data[k]["name"]][0] for k in group["data"]}
        self.assertTrue(
            {
                "B_SAY",
                "ITMI_SAMPLE",
                "ITMI_OTHER",
                "CREATEINVITEMS",
                "C_ITEM",
                "UBC_AMOUNT",
            }
            <= data
        )
        amount = next(r for r in profile.data if r["index"] == 3)
        self.assertEqual((amount["check_value"], amount["value"]), (True, 3))
        recipe = next(r for r in profile.functions if r["index"] == self.ids["recipe"])
        start, end = self.dat.extent(self.dat.symbols[self.ids["recipe"]])
        expected = fingerprint.fingerprint(
            self.dat.code,
            start,
            end - start,
            fingerprint.Header("UBC_RECIPE"),
            script_profile.SymbolNames(self.dat),
            script_profile.EntryNames(self.dat),
            [5, 25, 45],
            True,
        )
        self.assertEqual(recipe["canonical"], expected.canonical)
        self.assertEqual(recipe["raw"], expected.raw)

    def test_catalog_literals_join_the_station_share(self):
        say = self.dat.symbols[self.ids["say"]]
        profile = self.build(self.say_literals())
        share = profile.stations[0]
        say_id = next(
            k for k, r in enumerate(profile.functions) if r["index"] == say.index
        )
        self.assertIn(say_id, share["functions"])
        self.assertNotIn(say_id, profile.groups[0]["functions"])
        self.assertEqual(
            profile.anonymous_texts,
            [{"reference_index": self.ids["text"], "function": say_id, "offset": 0}],
        )
        self.assertFalse(profile.functions[say_id]["ignore_text"])

    def test_bound_text_keeps_exact_bytes(self):
        script, ids = synthetic(b"Fertig\xffgut")
        self.ids, self.dat = ids, DatFile.parse(script.bytes())
        self.assertEqual(self.dat.symbols[ids["text"]].content, [b"Fertig\xffgut"])
        self.build()  # unbound text: only its kind enters the digest
        with self.assertRaises(script_profile.ProfileError):
            self.build(self.say_literals())

    def test_render_is_deterministic_and_holds_no_code(self):
        first = script_profile.render(self.build(), CONFIG)
        second = script_profile.render(self.build(), CONFIG)
        self.assertEqual(first, second)
        self.assertIn('{"UBC_K\\344SE", 5},', first)
        self.assertNotIn("Fertig", first)
        self.assertTrue(first.isascii())

    def test_unmatched_verifiers_fail(self):
        recipes = recorded(self.dat, self.ids)
        recipes[0]["matches"] = False
        builder = script_profile.Builder("synthetic", self.dat, recipes, ([], []))
        with self.assertRaises(script_profile.ProfileError):
            builder.build()


def _unescape(text):
    return re.sub(
        r"\\([0-7]{3}|.)", lambda m: chr(int(m[1], 8)) if len(m[1]) == 3 else m[1], text
    )


def parse_header(path):
    text = path.read_text(encoding="ascii")

    def block(label):
        match = re.search(r"> " + label + r"\{\{\n(.*?)\n\}\};", text, re.S)
        return match[1] if match else ""

    names = [
        (_unescape(m[1]), int(m[2]))
        for m in re.finditer(r'\{"((?:[^"\\]|\\.)*)", (\d+)\}', block("names"))
    ]
    functions = [
        m.groups()
        for m in re.finditer(
            r'\{(\d+), (\d+), h\("[0-9a-f]{64}"\), (\d+), h\("([0-9a-f]{64})"\), (\d+), (\d+), (\d+),',
            block("functions"),
        )
    ]
    data = [
        m.groups()
        for m in re.finditer(r"\{(\d+), (\d+), (\d+), (\d+), ", block("data"))
    ]
    texts = [
        int(m[1]) for m in re.finditer(r"\{(\d+), \d+, \d+\}", block("anonymous_texts"))
    ]
    return {
        "functions": {
            (
                names[int(n)][0],
                names[int(n)][1],
                int(start),
                int(length),
                raw,
                int(kind),
                int(flags),
                int(argc),
            )
            for n, length, start, raw, kind, flags, argc in functions
        },
        "data": {
            (names[int(n)][0], names[int(n)][1], int(kind), int(flags), int(elements))
            for n, kind, flags, elements in data
        },
        "texts": set(texts),
        "size": len(text.encode()),
    }


class CommittedProfileTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.headers = {name: parse_header(GENERATED / name) for name in HEADERS}
        cls.functions, cls.bindings = catalog_literals.scan(ROOT / "include")

    def test_every_catalog_function_has_a_record(self):
        self.assertGreater(len(self.functions), 700)
        missing = []
        for f in self.functions:
            key = (
                f.name,
                f.symbol,
                f.start,
                f.length,
                f.raw_sha256,
                f.type,
                f.flags,
                f.argc,
            )
            if not any(key in header["functions"] for header in self.headers.values()):
                missing.append(f"{f.source}: {f.name}")
        self.assertEqual(missing, [])

    def test_every_catalog_binding_has_a_record(self):
        self.assertGreater(len(self.bindings), 2000)
        missing = []
        for b in self.bindings:
            if b.name.startswith("\xff"):
                found = any(
                    b.symbol in header["texts"] for header in self.headers.values()
                )
            else:
                key = (b.name, b.symbol, b.type, b.flags, b.elements)
                found = any(key in header["data"] for header in self.headers.values())
            if not found:
                missing.append(f"{b.source}: {b.name!r}")
        self.assertEqual(missing, [])

    def test_header_size(self):
        for name, header in self.headers.items():
            with self.subTest(header=name):
                self.assertLess(header["size"], HOOK_LIMIT)
                self.assertGreater(len(header["functions"]), 100)


if __name__ == "__main__":
    unittest.main()
