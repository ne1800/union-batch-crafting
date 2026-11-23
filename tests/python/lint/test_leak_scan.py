"""Run the scanner in a fresh repository with and without local settings."""

import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

SCAN = Path(__file__).resolve().parents[3] / "tools/lint/leak-scan.py"


class LeakScanTests(unittest.TestCase):
    def setUp(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self.root = Path(tmp.name)
        self.git("init", "--template=")
        self.sample = "fixture-value-for-local-rule"
        self.config = self.root / ".git" / "fixture-rules"
        self.config.write_text(json.dumps([self.sample]))
        self.tracked = self.root / "example"
        self.tracked.write_text(self.sample)
        self.git("add", "example")

    def git(self, *args):
        return subprocess.check_output(["git", *args], cwd=self.root, text=True).strip()

    def scan(self, *, use_env=False):
        env = os.environ.copy()
        env.pop("UBC_LEAK_PATTERNS", None)
        if use_env:
            env["UBC_LEAK_PATTERNS"] = str(self.config)
        result = subprocess.run(
            [sys.executable, "-B", SCAN],
            cwd=self.root,
            env=env,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )
        self.assertNotIn(self.sample, result.stdout)
        self.assertNotIn(str(self.config), result.stdout)
        return result

    def test_general_rules_without_configuration(self):
        self.assertEqual(self.scan().returncode, 0)
        self.tracked.write_text("place@" + "example.invalid")
        self.assertIn("email address", self.scan().stdout)

    def test_git_setting_and_environment(self):
        self.assertNotEqual(self.scan(use_env=True).returncode, 0)
        self.git("config", "--local", "ubc.leakPatterns", str(self.config))
        self.assertIn("local pattern", self.scan().stdout)
        original = self.git("rev-parse", ":example")
        self.tracked.write_text("clean")
        self.git("add", "example")
        self.assertIn("local pattern", self.scan().stdout)  # Unreachable first blob.
        self.git("replace", original, self.git("rev-parse", ":example"))
        self.assertIn("local pattern", self.scan().stdout)

    def test_unreadable_and_malformed_configuration(self):
        self.git("config", "--local", "ubc.leakPatterns", str(self.config))
        self.config.unlink()
        self.assertNotEqual(self.scan().returncode, 0)
        self.config.write_text(json.dumps(["(" + self.sample]))
        self.assertIn("Cannot load local scan patterns", self.scan().stdout)

    def test_plain_webp_screenshots_only(self):
        def webp(*chunks):
            body = b"WEBP" + b"".join(
                kind + len(data).to_bytes(4, "little") + data + b"\0" * (len(data) & 1)
                for kind, data in chunks
            )
            return b"RIFF" + len(body).to_bytes(4, "little") + body

        images = self.root / "docs" / "images"
        images.mkdir(parents=True)
        plain = images / "menu-en.webp"
        plain.write_bytes(webp((b"VP8 ", b"\x00\x01pixels\x7f")))
        self.git("add", "docs")
        self.assertEqual(self.scan().returncode, 0)
        elsewhere = self.root / "menu.webp"
        elsewhere.write_bytes(plain.read_bytes())
        self.git("add", "menu.webp")
        self.assertIn("binary", self.scan().stdout)
        self.git("rm", "-q", "--cached", "menu.webp")
        elsewhere.unlink()
        plain.write_bytes(
            webp(
                (b"VP8X", b"\x08" + b"\0" * 9),
                (b"EXIF", b"camera\0"),
                (b"VP8 ", b"\x00"),
            )
        )
        self.git("add", "docs")
        self.assertIn("binary", self.scan().stdout)
