"""Safeguards of the write-enabled helpers, using entirely our own test data."""
import contextlib
import io
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import types
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]


def load_tool(name):
    path = ROOT / "tools" / name
    module = types.ModuleType(name)
    module.__file__ = str(path)
    # As with an actual CLI invocation, helpers can import adjacent modules without
    # relying on an incidental sys.path change from a previously loaded test suite.
    with patch.object(sys, "path", [str(path.parent), *sys.path]):
        exec(compile(path.read_text(), str(path), "exec"), module.__dict__)
    return module


class ToolTests(unittest.TestCase):
    def test_setup_dry_run_creates_nothing(self):
        tool = load_tool("setup-toolchain-WRITES.py")
        with tempfile.TemporaryDirectory(prefix="gothic-batch-test.", dir="/tmp") as directory:
            root = Path(directory)
            (root / ".git").mkdir()
            with patch.object(tool, "ROOT", root), patch.object(sys, "argv", ["setup"]), contextlib.redirect_stdout(io.StringIO()):
                tool.main()
            self.assertEqual([".git"], sorted(p.name for p in root.iterdir()))

    def test_setup_rejects_unknown_archive_without_network(self):
        tool = load_tool("setup-toolchain-WRITES.py")
        with tempfile.TemporaryDirectory(prefix="gothic-batch-test.", dir="/tmp") as directory:
            root = Path(directory)
            (root / ".git").mkdir()
            (root / ".tools").mkdir()
            (root / ".tools" / tool.ARCHIVE_NAME).write_bytes(b"untrusted")
            with patch.object(tool, "ROOT", root), patch.object(sys, "argv", ["setup", "--apply"]), patch.object(tool.urllib.request, "urlopen") as network:
                with self.assertRaises(SystemExit):
                    tool.main()
                network.assert_not_called()

    def test_setup_rejects_dangling_archive_symlink(self):
        tool = load_tool("setup-toolchain-WRITES.py")
        with tempfile.TemporaryDirectory(prefix="gothic-batch-test.", dir="/tmp") as directory:
            root = Path(directory)
            (root / ".git").mkdir()
            (root / ".tools").mkdir()
            (root / ".tools" / tool.ARCHIVE_NAME).symlink_to(root / "outside")
            with patch.object(tool, "ROOT", root), patch.object(sys, "argv", ["setup", "--apply"]):
                with self.assertRaises(SystemExit):
                    tool.main()
            self.assertFalse((root / "outside").exists())

    def test_smoke_dry_run_never_starts_process(self):
        tool = load_tool("smoke-test-WRITES.py")
        with patch.object(sys, "argv", ["smoke"]), patch.object(tool.subprocess, "run") as process, contextlib.redirect_stdout(io.StringIO()):
            tool.main()
            process.assert_not_called()

    def test_smoke_rejects_foreign_or_symlink_directory(self):
        tool = load_tool("smoke-test-WRITES.py")
        with self.assertRaises(RuntimeError):
            tool.validate_temporary(Path("/tmp"))
        with tempfile.TemporaryDirectory(prefix="gothic-batch-test.", dir="/tmp") as directory:
            root = Path(directory)
            tool.validate_temporary(root)
            link = root / "link"
            link.symlink_to(root, target_is_directory=True)
            with self.assertRaises(RuntimeError):
                tool.validate_temporary(link)

    def test_failed_wineserver_cleanup_preserves_prefix(self):
        tool = load_tool("smoke-test-WRITES.py")
        real_mkdtemp = tempfile.mkdtemp
        for failing_option in ("-k", "-w"):
            with self.subTest(failing_option=failing_option), tempfile.TemporaryDirectory(prefix="gothic-batch-test.", dir="/tmp") as directory:
                root = Path(directory)
                build = root / "build/windows"
                build.mkdir(parents=True)
                (build / "UnionBatchCraftingProbe.dll").write_bytes(b"own-fixture-dll")
                (build / "ubc_core_tests.exe").write_bytes(b"own-fixture-core")
                executable = bytearray(512)
                executable[:2] = b"MZ"
                struct.pack_into("<I", executable, 0x3c, 128)
                executable[128:132] = b"PE\0\0"
                struct.pack_into("<I", executable, 128 + 24 + 28, 0x400000)
                (build / "ubc_probe_host.exe").write_bytes(executable)
                created = []

                def create_directory(*args, **kwargs):
                    value = real_mkdtemp(*args, **kwargs)
                    created.append(Path(value))
                    return value

                def process(arguments, **kwargs):
                    environment = kwargs["env"]
                    self.assertEqual(environment["WINEPREFIX"], str(created[0] / "prefix"))
                    if arguments[0] == "/fake/wineserver":
                        return subprocess.CompletedProcess(arguments, int(arguments[1] == failing_option))
                    if arguments[-1] == "1":
                        log = created[0] / "Probe Ä mit Leerzeichen/UnionBatchCraftingProbe.log"
                        log.write_text("probe_version=1\ntarget_header_match=true\ngameplay_hooks=false\nscript_calls=false\n")
                    return subprocess.CompletedProcess(arguments, 0, stdout="synthetic test")

                try:
                    with patch.object(tool, "ROOT", root), patch.object(sys, "argv", ["smoke", "--apply"]), patch.object(tool.shutil, "which", side_effect=lambda name: "/fake/" + name), patch.object(tool.tempfile, "mkdtemp", side_effect=create_directory), patch.object(tool.subprocess, "run", side_effect=process), contextlib.redirect_stdout(io.StringIO()):
                        with self.assertRaisesRegex(RuntimeError, "Wine-Aufräumen fehlgeschlagen"):
                            tool.main()
                    self.assertEqual(len(created), 1)
                    self.assertTrue(created[0].is_dir())
                    self.assertTrue((created[0] / "wine.log").is_file())
                finally:
                    for path in created:
                        tool.validate_temporary(path)
                        shutil.rmtree(path)


if __name__ == "__main__":
    unittest.main()
