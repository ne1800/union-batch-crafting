#!/usr/bin/env python3
"""Prepare local Windows build tools. Writing requires --apply."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tarfile
import urllib.request

ROOT = Path(__file__).resolve().parents[2]
VERSION = "0.10.0"
ARCHIVE_NAME = f"xwin-{VERSION}-x86_64-unknown-linux-musl.tar.gz"
ARCHIVE_SHA = "d870eb4b2f390878af6da1ccd3cf321d22fcb72720984853b4be732ae597fc88"
URL = f"https://github.com/Jake-Shadle/xwin/releases/download/{VERSION}/{ARCHIVE_NAME}"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--apply",
        action="store_true",
        help="Download the tools and write the local SDK",
    )
    args = parser.parse_args()
    target = ROOT / ".tools"
    if target.resolve() != target or not (ROOT / ".git").is_dir():
        raise SystemExit("Unsicheres Projekt-/Werkzeugverzeichnis.")
    if not args.apply:
        print(
            f"Plan: xwin {VERSION} und Windows-x86-SDK unter {target}; Ausführung nur mit --apply."
        )
        return
    target.mkdir(exist_ok=True)
    archive = target / ARCHIVE_NAME
    for path in (
        archive,
        target / "xwin-bin",
        target / "toolchain-lock.json",
        target / "setup.log",
    ):
        if path.is_symlink():
            raise SystemExit("Werkzeugdateien dürfen keine Symlinks sein.")
    if archive.exists() and (
        archive.is_symlink()
        or hashlib.sha256(archive.read_bytes()).hexdigest() != ARCHIVE_SHA
    ):
        raise SystemExit("Vorhandenes Werkzeugarchiv hat einen unerwarteten Hash.")
    if not archive.exists():
        request = urllib.request.Request(
            URL, headers={"User-Agent": "UnionBatchCrafting-build/0.0.1"}
        )
        with urllib.request.urlopen(request, timeout=60) as response:
            data = response.read(16 * 1024 * 1024 + 1)
        if (
            len(data) > 16 * 1024 * 1024
            or hashlib.sha256(data).hexdigest() != ARCHIVE_SHA
        ):
            raise SystemExit("Werkzeugdownload: Größe oder SHA-256 stimmt nicht.")
        archive.write_bytes(data)
    with tarfile.open(archive, "r:gz") as bundle:
        members = [
            m for m in bundle.getmembers() if m.isfile() and Path(m.name).name == "xwin"
        ]
        if len(members) != 1:
            raise SystemExit("Werkzeugarchiv enthält nicht genau eine xwin-Datei.")
        stream = bundle.extractfile(members[0])
        if stream is None or members[0].size > 32 * 1024 * 1024:
            raise SystemExit("Ungültiges xwin-Archivmitglied.")
        binary_data = stream.read()
    binary = target / "xwin-bin"
    if binary.exists() and (binary.is_symlink() or binary.read_bytes() != binary_data):
        raise SystemExit("Unbekannte vorhandene xwin-Binärdatei wird nicht ausgeführt.")
    if not binary.exists():
        binary.write_bytes(binary_data)
    binary.chmod(0o755)
    sdk = target / "xwin"
    cache = target / "xwin-cache"
    for path in (sdk, cache):
        if path.resolve() != path:
            raise SystemExit("SDK-/Cache-Ziel darf kein Symlink sein.")
    lock = target / "toolchain-lock.json"
    pinned_manifest = ROOT / "tools/build/windows-sdk-manifest.json"
    pinned_manifest_sha = hashlib.sha256(pinned_manifest.read_bytes()).hexdigest()
    expected_files = [
        sdk / "crt/include/vcruntime.h",
        sdk / "sdk/include/um/windows.h",
        sdk / "sdk/lib/um/x86/kernel32.lib",
        sdk / "sdk/lib/ucrt/x86/libucrt.lib",
    ]
    previous = json.loads(lock.read_text()) if lock.exists() else {}
    if previous.get("pinned_manifest_sha256") != pinned_manifest_sha or not all(
        p.is_file() for p in expected_files
    ):
        print(
            "Windows-x86-SDK wird lokal vorbereitet; Fortschritt in .tools/setup.log.",
            flush=True,
        )
        with (target / "setup.log").open("w") as log:
            subprocess.run(
                [
                    str(binary),
                    "--arch",
                    "x86",
                    "--accept-license",
                    "--cache-dir",
                    str(cache),
                    "--manifest",
                    str(pinned_manifest),
                    "splat",
                    "--output",
                    str(sdk),
                ],
                stdout=log,
                stderr=subprocess.STDOUT,
                timeout=1800,
                check=True,
            )
        if not all(p.is_file() for p in expected_files):
            raise SystemExit("SDK unvollständig; siehe .tools/setup.log.")
        manifests = {}
        for p in sorted(cache.rglob("*")):
            if p.is_file() and p.suffix in (".json", ".vsman"):
                manifests[str(p.relative_to(cache))] = hashlib.sha256(
                    p.read_bytes()
                ).hexdigest()
        record = {
            "xwin_version": VERSION,
            "xwin_archive_sha256": ARCHIVE_SHA,
            "xwin_binary_sha256": hashlib.sha256(binary_data).hexdigest(),
            "architecture": "x86",
            "cached_manifests": manifests,
            "pinned_manifest_sha256": pinned_manifest_sha,
            "sdk_check_files": {
                str(p.relative_to(sdk)): hashlib.sha256(p.read_bytes()).hexdigest()
                for p in expected_files
            },
        }
        lock.write_text(json.dumps(record, indent=2) + "\n")
    record = json.loads(lock.read_text())
    if record.get("xwin_archive_sha256") != ARCHIVE_SHA:
        raise SystemExit("Werkzeug-Lock stimmt nicht mit dem geprüften Archiv überein.")
    for path, expected in record["sdk_check_files"].items():
        candidate = sdk / path
        if (
            not candidate.resolve().is_relative_to(sdk)
            or hashlib.sha256(candidate.read_bytes()).hexdigest() != expected
        ):
            raise SystemExit("SDK-Prüfdatei stimmt nicht mit dem lokalen Lock überein.")
    print("Windows-x86-SDK bereit; lokale Prüfsummen stimmen.")


if __name__ == "__main__":
    main()
