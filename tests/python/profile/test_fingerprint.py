"""UBCF1 reference fingerprints over synthetic bytecode only."""

from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "tools/profile"))
from fingerprint import FingerprintError, Header, Text, fingerprint

# Shared with tests/native/engine/script_fingerprint_tests.cpp. Both
# implementations must reproduce these digests for the golden function.
GOLDEN = "e9d47319934443fa81ae089ec9508f35882ace20ec13b2f0ebfd23da95a77acc"
GOLDEN_WITHOUT_TEXT = "c20427c8ec8fc151436a48aaf619dff4fd35d821af290ba83117da6c1046f030"
START = 100


def word(opcode, value):
    return bytes((opcode,)) + struct.pack("<i", value)


def golden_body(start=START, symbol=1234, external=3, text=4, callee=500, array=5):
    return b"".join(
        (
            word(64, 7),
            word(64, symbol),
            word(62, external),
            word(65, text),
            word(61, callee),
            word(245, array) + bytes((2,)),
            word(76, start + 42),
            bytes((9,)),
            word(75, start + 43),
            bytes((60, 60)),
        )
    )


def golden_symbols(symbol=1234, external=3, text=4, array=5, content=b"Hallo Welt"):
    return {
        symbol: "ITFO_APPLE",
        external: "CREATEINVITEMS",
        text: Text(content),
        array: "UBC_ARRAY",
    }


HEADER = Header("UBC_GOLDEN")


def run(body, start=START, symbols=None, entries=None, symbolic=(5,), **options):
    code = bytes(start) + body + bytes((60,) * 8)
    return fingerprint(
        code,
        start,
        len(body),
        options.pop("header", HEADER),
        golden_symbols() if symbols is None else symbols,
        {500: "B_SAY"} if entries is None else entries,
        symbolic,
        **options,
    )


class FingerprintTests(unittest.TestCase):
    def assert_issue(self, issue, offset, body, **options):
        with self.assertRaises(FingerprintError) as raised:
            run(body, **options)
        self.assertEqual(
            (raised.exception.issue, raised.exception.offset), (issue, offset)
        )

    def test_golden_digest(self):
        result = run(golden_body())
        self.assertEqual(result.canonical, GOLDEN)
        self.assertEqual(result.instructions, 11)
        self.assertEqual(
            run(golden_body(), ignore_text=True).canonical, GOLDEN_WITHOUT_TEXT
        )

    def test_relocation_keeps_canonical_and_changes_raw(self):
        base = run(golden_body())
        moved = run(
            golden_body(
                start=900, symbol=2234, external=13, text=14, callee=4100, array=15
            ),
            start=900,
            symbols=golden_symbols(2234, 13, 14, 15),
            entries={4100: "B_SAY"},
        )
        self.assertEqual(moved.canonical, base.canonical)
        self.assertNotEqual(moved.raw, base.raw)

    def test_literal_and_text_changes(self):
        base = run(golden_body()).canonical
        changed = bytearray(golden_body())
        changed[1] = 8
        self.assertNotEqual(run(bytes(changed)).canonical, base)
        other_text = run(golden_body(), symbols=golden_symbols(content=b"Hallo Welt!"))
        self.assertNotEqual(other_text.canonical, base)
        ignored = run(
            golden_body(),
            symbols=golden_symbols(content=b"Hallo Welt!"),
            ignore_text=True,
        )
        self.assertEqual(ignored.canonical, GOLDEN_WITHOUT_TEXT)
        self.assertNotEqual(run(golden_body(), symbolic=()).canonical, base)
        renamed = run(golden_body(), header=Header("UBC_OTHER"))
        self.assertNotEqual(renamed.canonical, base)

    def test_rejections(self):
        body = golden_body()
        self.assert_issue(
            "jump_target", 31, body[:31] + word(76, START + 44) + body[36:]
        )
        self.assert_issue(
            "jump_target", 0, word(75, START - 1) + bytes((60,)), symbolic=()
        )
        mid = body[:31] + word(76, START + 38) + body[36:]
        self.assert_issue("jump_target", 38, mid)
        self.assert_issue("unknown_opcode", 36, body[:36] + bytes((10,)) + body[37:])
        self.assert_issue("truncated_instruction", 0, word(64, 1)[:3], symbolic=())
        self.assert_issue("missing_ret", 7, word(64, 1) + bytes((60, 9)), symbolic=())
        self.assert_issue("symbolic_int", 10, body, symbolic=(5, 10))
        self.assert_issue("symbolic_int", 6, body, symbolic=(5, 6))
        self.assert_issue("unknown_callee", 20, body, entries={501: "B_SAY"})
        unknown = golden_symbols()
        del unknown[5]
        self.assert_issue("unknown_symbol", 25, body, symbols=unknown)
        self.assert_issue("invalid_input", 0, body, symbolic=(10, 5))
        self.assert_issue("function_length", 0, b"", symbolic=())


if __name__ == "__main__":
    unittest.main()
