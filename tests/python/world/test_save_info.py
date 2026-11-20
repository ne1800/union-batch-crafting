"""Bind the world without publishing the remaining save metadata."""

import sys
from pathlib import Path
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "tools"))
from world.save_info import saved_world


class SavedWorldTests(unittest.TestCase):
    def info(self, world="OLDWORLD"):
        lines = [
            "ZenGin Archive",
            "ver 1",
            "zCArchiverGeneric",
            "ASCII",
            "saveGame 0",
            "date private",
            "user private",
            "END",
            "objects 1",
            "END",
            "[% oCSavegameInfo 0 0]",
            "Title=string:private",
            "WorldName=string:" + world,
            "TimeDay=int:3",
            "TimeHour=int:12",
            "TimeMin=int:0",
            "SaveDate=string:private",
            "VersionMajor=int:2",
            "VersionMinor=int:6",
            "PlayTimeSeconds=int:123",
            "VersionPoint=int:0",
            "VersionInt=int:0",
            "VersionAppName=string:private",
            "[]",
        ]
        return ("\r\n".join(lines) + "\r\n").encode("cp1252")

    def test_only_normalized_world_leaves_parser(self):
        for value, expected in [
            ("OLDWORLD", "OLDWORLD"),
            ("NEWWORLD.ZEN", "NEWWORLD"),
            ("ADDONWORLD", "ADDONWORLD"),
        ]:
            self.assertEqual(saved_world(self.info(value)), expected)

    def test_rejects_wrong_archive_or_ambiguous_field(self):
        data = self.info()
        for old, new in [
            (b"ASCII", b"BINARY"),
            (b"objects 1", b"objects 2"),
            (b"saveGame 0", b"saveGame 1"),
            (b"oCSavegameInfo 0 0", b"oCSavegameInfo 1 0"),
            (b"WorldName=string:OLDWORLD", b"WorldName=int:0"),
            (b"Title=string:private", b"WorldName=string:NEWWORLD"),
            (b"TimeDay=int:3", b"TimeDay=int:2147483648"),
            (b"TimeDay=int:3", b"TimeDay=int:1\x00"),
            (b"[]", b"[]\r\ntrailing"),
        ]:
            with self.subTest(new=new):
                with self.assertRaises(ValueError):
                    saved_world(data.replace(old, new))

    def test_rejects_missing_nested_or_unknown_fields(self):
        for replacement in (b"", b"Unknown=string:private", b"[% oCSavegameInfo 0 1]"):
            with self.assertRaises(ValueError):
                saved_world(self.info().replace(b"Title=string:private", replacement))
        with self.assertRaises(ValueError):
            saved_world(b"x" * 65537)

    def test_rejects_world_paths_and_control_text(self):
        for world in (
            "",
            "../OLDWORLD",
            "oldworld",
            "OLDWORLD.ZEN/elsewhere",
            "OLDWORLD\nWorldName=string:NEWWORLD",
        ):
            with self.assertRaises(ValueError):
                saved_world(self.info(world))


if __name__ == "__main__":
    unittest.main()
