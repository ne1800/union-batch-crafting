"""Exercise VDFS directory links, stable bytes and Workshop content verification."""

import hashlib
import os
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch

from tools.package import vdf, workshop


class WorkshopArchive(unittest.TestCase):
    def test_round_trip_and_fixed_catalogue_layout(self):
        # A one-byte payload makes every absolute offset visible. The two root
        # siblings must precede the children of SYSTEM, just as in plugin VDFs.
        members = {"SYSTEM/AUTORUN/PLUGIN.DLL": b"A", "README": b"B"}
        raw = vdf.pack(members)
        self.assertEqual(raw, vdf.pack(dict(reversed(members.items()))))
        self.assertEqual(vdf.read_volume(raw)[1], members)
        self.assertEqual(raw[256:272], b"PSVDSC_V2.00\n\r\n\r")
        self.assertEqual(
            struct.unpack_from("<6I", raw, 272), (4, 2, 0x28210000, 2, 296, 80)
        )
        # Read the disk fields directly, independently of the packer's tree walk.
        catalogue = []
        for index in range(4):
            row = raw[296 + 80 * index : 296 + 80 * (index + 1)]
            catalogue.append((row[:64].rstrip(b" "), *struct.unpack("<4I", row[64:])))
        self.assertEqual(
            catalogue,
            [
                (b"README", 616, 1, 0, 32),
                (b"SYSTEM", 2, 0, 0xC0000000, 0),
                (b"AUTORUN", 3, 0, 0xC0000000, 0),
                (b"PLUGIN.DLL", 617, 1, 0x40000000, 32),
            ],
        )
        self.assertEqual(raw[616:], b"BA")

    def test_broken_catalogues_and_path_conflicts(self):
        raw = vdf.pack({"SYSTEM/PLUGIN.DLL": b"x"})
        for offset, value in (
            (296 + 64, 0),
            (296 + 64, 7),
            (376 + 64, 1),
            (376 + 72, 0),
        ):
            broken = bytearray(raw)
            struct.pack_into("<I", broken, offset, value)
            with (
                self.subTest(offset=offset, value=value),
                self.assertRaises(ValueError),
            ):
                vdf.read_volume(broken)
        for files in (
            {"A/B": b"x", "A": b"y"},
            {"A": b"y", "A/B": b"x"},
            {"../D": b"x"},
        ):
            with self.assertRaises(ValueError):
                vdf.pack(files)

    def test_pinned_package_and_failed_rebuild_preserves_output(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = root / "build/windows/player" / workshop.DLL
            source.parent.mkdir(parents=True)
            data = bytearray(512)
            data[:2] = b"MZ"
            struct.pack_into("<I", data, 60, 128)
            data[128:132] = b"PE\0\0"
            for offset, value in ((132, 0x14C), (150, 0x2000), (152, 0x10B)):
                struct.pack_into("<H", data, offset, value)
            data.extend("1.2.3\0Enabled\0".encode("utf-16le"))
            source.write_bytes(data)
            (root / "VERSION").write_text("1.2.3")
            pins = root / "tools/ci/dll.sha256"
            pins.parent.mkdir(parents=True)
            digest = hashlib.sha256(data).hexdigest()
            pins.write_text(f"{digest}  player/{workshop.DLL}\n")
            for name in workshop.NOTICES:
                (root / name).write_bytes(name.encode())
            with (
                patch.object(workshop, "ROOT", root),
                patch("sys.argv", ["workshop", "build"]),
            ):
                workshop.main()
            self.assertFalse((root / "dist").exists())
            archive = workshop.build(root)
            raw = archive.read_bytes()
            self.assertEqual(workshop.verify(root, raw), digest)
            self.assertEqual(workshop.build(root).read_bytes(), raw)
            members = vdf.read_volume(raw)[1]
            with self.assertRaises(ValueError):
                workshop.verify(
                    root, vdf.pack({**members, workshop.MEMBER: data + b"!"})
                )
            with self.assertRaises(ValueError):
                workshop.verify(root, vdf.pack({**members, "EXTRA": b"x"}))
            with self.assertRaises(ValueError):
                workshop.verify(root, raw + b"extra")
            (archive.parent.parent / "old.txt").touch()
            with self.assertRaises(ValueError):
                workshop.check_content_tree(archive)
            (archive.parent.parent / "old.txt").unlink()
            source.write_bytes(data + b"!")
            with self.assertRaises(ValueError):
                workshop.build(root)
            self.assertEqual(archive.read_bytes(), raw)

    @unittest.skipUnless(
        os.getenv("UBC_SCREENINFO_VDF"), "optional installed Workshop reference"
    )
    def test_installed_screeninfo_archive(self):
        # Item 2787243474. The game archive stays local and read-only.
        data = Path(os.environ["UBC_SCREENINFO_VDF"]).read_bytes()
        (_, _, _), files = vdf.read_volume(data)
        self.assertTrue(
            files["SYSTEM/AUTORUN/UNION_GOTHIC_SCREEN_INFO.DLL"].startswith(b"MZ")
        )
        self.assertIn("SYSTEM/AUTORUN/MENU/ZUNIONMENU_HEDIN.D", files)
        self.assertIn("_WORK/DATA/TEXTURES/_COMPILED/BG_MUNITION_ARROWS-C.TEX", files)
        self.assertEqual(vdf.read_volume(vdf.pack(files))[1], files)
