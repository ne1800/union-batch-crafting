"""Check the player archive, wrong-flavour rejection and bilingual install paths."""

from pathlib import Path
import runpy
import struct
import tempfile
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[3]
api = runpy.run_path(str(ROOT / "tools/package/package-player-WRITES.py"))
DLL = "UnionBatchCraftingExtended.dll"
MEMBER = "system/autorun/" + DLL
DOCS = {
    "LICENSE",
    "THIRD-PARTY-NOTICES",
    "TESTED-WITH.md",
    "INSTALL.de.md",
    "INSTALL.md",
}


class PlayerPackage(unittest.TestCase):
    def test_archive_and_variant(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            dll = root / "build/windows/player" / DLL
            dll.parent.mkdir(parents=True)
            data = bytearray(512)
            data[:2] = b"MZ"
            struct.pack_into("<I", data, 60, 128)
            data[128:132] = b"PE\0\0"
            for offset, value in ((132, 0x14C), (150, 0x2000), (152, 0x10B)):
                struct.pack_into("<H", data, offset, value)
            data.extend("0.0.57\0Enabled\0".encode("utf-16le"))
            dll.write_bytes(data)
            for name in DOCS:
                (root / name).write_bytes((ROOT / name).read_bytes())
            with zipfile.ZipFile(api["package"](root)) as archive:
                self.assertCountEqual(archive.namelist(), [MEMBER, *DOCS])
                self.assertEqual(archive.read(MEMBER), data)
                self.assertIsNone(archive.testzip())
                for name in DOCS:
                    self.assertEqual(archive.read(name), (ROOT / name).read_bytes())
                for name in ("INSTALL.md", "INSTALL.de.md"):
                    guide = archive.read(name).decode()
                    for literal in (
                        MEMBER,
                        "Enabled=0",
                        "Enabled=1",
                        "Language=auto",
                        "Gothic.INI",
                        ".log",
                    ):
                        self.assertIn(literal, guide)
            dll.write_bytes(data + ".enable-once".encode("utf-16le"))
            with self.assertRaises(ValueError):
                api["package"](root)
