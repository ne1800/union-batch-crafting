"""Check the release metadata, the version helper and the release notes."""

import contextlib
import hashlib
import io
from pathlib import Path
import runpy
import shutil
import tempfile
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[3]
release = runpy.run_path(str(ROOT / "tools/release/release.py"))
prepare = runpy.run_path(str(ROOT / "tools/release/prepare-WRITES.py"))
FILES = (
    "VERSION",
    "INSTALL.md",
    "INSTALL.de.md",
    "THIRD-PARTY-NOTICES",
    "CHANGELOG.md",
)
DLLS = ("player/UnionBatchCraftingExtended.dll", "UnionBatchCraftingExtended.dll")


def write(changes):
    for path, text in changes.items():
        path.write_text(text, encoding="utf-8")


class Release(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.root = Path(self.directory.name)
        for name in FILES:
            shutil.copy(ROOT / name, self.root / name)
        (self.root / "tools/ci").mkdir(parents=True)
        (self.root / "TESTED-WITH.md").write_text(
            "| Component | SHA-256 |\n", encoding="utf-8"
        )
        for number, name in enumerate(DLLS):
            dll = self.root / "build/windows" / name
            dll.parent.mkdir(parents=True, exist_ok=True)
            dll.write_bytes(b"MZ dll %d" % number)

    def tearDown(self):
        self.directory.cleanup()

    def start(self, version="9.8.7"):
        write(prepare["versioned_files"](self.root, version))
        write(prepare["pinned_hashes"](self.root))

    def describe(self, version="9.8.7"):
        changelog = self.root / "CHANGELOG.md"
        text = changelog.read_text(encoding="utf-8")
        for stub, entry in zip(
            prepare["STUBS"].values(), ("Faster crafting.", "Schneller.")
        ):
            text = text.replace(f"- **{version}** {stub}", f"- **{version}** {entry}")
        changelog.write_text(text, encoding="utf-8")

    def record_test(self, version="9.8.7"):
        digest = hashlib.sha256(b"MZ dll 0").hexdigest()
        with (self.root / "TESTED-WITH.md").open("a", encoding="utf-8") as tested:
            tested.write(f"| Player DLL / Spieler-DLL ({version}) | `{digest}` |\n")

    def archive(self, dll=b"MZ dll 0", version="9.8.7"):
        path = self.root / f"union-batch-crafting-player-{version}.zip"
        with zipfile.ZipFile(path, "w") as archive:
            archive.writestr(release["DLL_MEMBER"], dll)
        return path

    def test_repository_names_one_version(self):
        current = release["version"](ROOT)
        stubs = prepare["STUBS"]
        expected = []
        if release["tested_dll"](ROOT, current) is None and all(
            release["changelog_entry"](ROOT, current, language) == stub
            for language, stub in stubs.items()
        ):
            # An untested version prepared locally must still fail the release check.
            expected = [
                f"CHANGELOG.md: describe {current} in the {language} section"
                for language in stubs
            ]
        self.assertEqual(release["consistency_errors"](ROOT), expected)

    def test_new_version_waits_for_changelog_and_test(self):
        self.start()
        self.assertEqual(release["version"](self.root), "9.8.7")
        errors = release["consistency_errors"](self.root)
        self.assertEqual(len(errors), 2, errors)
        self.assertTrue(all("CHANGELOG.md" in error for error in errors))
        self.describe()
        self.assertEqual(release["consistency_errors"](self.root), [])
        self.assertIn("record the player DLL", release["evidence_errors"](self.root)[0])
        self.record_test()
        self.assertEqual(release["evidence_errors"](self.root), [])

    def test_changed_dll_after_test_blocks_release(self):
        self.start()
        self.describe()
        self.record_test()
        (self.root / "build/windows" / DLLS[0]).write_bytes(b"MZ changed")
        write(prepare["pinned_hashes"](self.root))
        self.assertIn("in-game test used", release["evidence_errors"](self.root)[0])

    def test_notes_list_entries_and_hashes(self):
        self.start()
        self.describe()
        self.record_test()
        archive = self.archive()
        notes = release["notes"](self.root, archive, "owner/name")
        self.assertIn("Faster crafting.\n\nSchneller.\n", notes)
        self.assertIn(hashlib.sha256(archive.read_bytes()).hexdigest(), notes)
        self.assertIn(hashlib.sha256(b"MZ dll 0").hexdigest(), notes)
        self.assertIn("https://github.com/owner/name/blob/v9.8.7/TESTED-WITH.md", notes)

    def test_notes_reject_an_untested_dll(self):
        self.start()
        self.describe()
        self.record_test()
        with self.assertRaisesRegex(ValueError, "not the one tested"):
            release["notes"](self.root, self.archive(b"MZ other"), "owner/name")

    def test_preview_changes_nothing(self):
        before = {name: (ROOT / name).read_bytes() for name in FILES}
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            prepare["main"](["--version", "9.8.7"])
        self.assertIn("Would write VERSION", output.getvalue())
        self.assertEqual(before, {name: (ROOT / name).read_bytes() for name in FILES})


if __name__ == "__main__":
    unittest.main()
