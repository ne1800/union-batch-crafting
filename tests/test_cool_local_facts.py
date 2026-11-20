"""The positive cooling check must not skip missing file contracts."""

from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from cool_local_facts import local_files


class CoolLocalFactsTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="gothic-batch-test.cool-files-")
        self.addCleanup(self.temp.cleanup)
        root = Path(self.temp.name)
        self.game = root / "common/Gothic II"
        self.workshop = root / "workshop/content/39510"
        self.files = [
            self.game / "system/Gothic2.exe",
            self.workshop / "2847787799/Data/ModVDF/Lhiver_marcello.mod",
            self.game / "Data/$Templates$/ZBUGFIXES.DLL",
            self.workshop / "2787458679/System/Shw32.dll",
            self.game / "system/msvcr100.dll",
            self.game / "Data/$Templates$/zParserExtender.dll",
            self.game / "system/gd3d11/bin/g2a_avx2.dll",
        ]
        for path in self.files:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes("Nur eigene Pfadfixture; kein Binärvertrag".encode())

    def test_exact_order_and_no_writes(self):
        before = {p: p.read_bytes() for p in self.files}
        self.assertEqual(local_files(*self.files[:2]), self.files)
        self.assertEqual(before, {p: p.read_bytes() for p in self.files})

    def test_missing_module_is_an_error(self):
        self.files[3].unlink()
        with self.assertRaises(FileNotFoundError):
            local_files(*self.files[:2])

    def test_directory_is_not_a_binary(self):
        self.files[4].unlink()
        self.files[4].mkdir()
        with self.assertRaises(ValueError):
            local_files(*self.files[:2])

    def test_linked_executable_keeps_installation_root(self):
        source = Path(self.temp.name) / "different-disk.exe"
        self.files[0].rename(source)
        self.files[0].symlink_to(source)
        self.assertEqual(local_files(*self.files[:2]), [source, *self.files[1:]])


if __name__ == "__main__":
    unittest.main()
