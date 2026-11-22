#!/usr/bin/env python3
"""Start a version or pin the built DLL hashes. Writes only with --apply.

--version sets VERSION, the versioned titles and ZIP names in the guides, the
first line of THIRD-PARTY-NOTICES and a changelog stub in both languages.
--pin-dlls writes the SHA-256 of both built DLLs to tools/ci/dll.sha256.
The in-game test result in TESTED-WITH.md stays a manual step.
"""

import argparse
import hashlib
from pathlib import Path
import re
import runpy
import sys

ROOT = Path(__file__).resolve().parents[2]
release = runpy.run_path(str(Path(__file__).with_name("release.py")))
STUBS = {
    "English": "TODO: describe the change for players.",
    "Deutsch": "TODO: die Änderung für Spieler beschreiben.",
}
DLLS = ("player/UnionBatchCraftingExtended.dll", "UnionBatchCraftingExtended.dll")


def versioned_files(root, new):
    """Return {path: new text} for every file that names the version."""
    old = release["version"](root)
    changes = {root / "VERSION": new + "\n"}
    for name, audience in release["GUIDES"].items():
        text = (root / name).read_text(encoding="utf-8")
        text = text.replace(
            f"# Union Batch Crafting {old} {audience}\n",
            f"# Union Batch Crafting {new} {audience}\n",
        )
        changes[root / name] = release["ARCHIVE"].sub(
            release["archive_name"](new), text
        )
    notices = (root / "THIRD-PARTY-NOTICES").read_text(encoding="utf-8")
    _, _, rest = notices.partition("\n")
    changes[root / "THIRD-PARTY-NOTICES"] = f"Union Batch Crafting {new}\n{rest}"
    changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")
    for language, stub in STUBS.items():
        if release["changelog_entry"](root, new, language) is None:
            heading = release["HEADINGS"][language] + "\n\n"
            if heading not in changelog:
                raise ValueError(f"CHANGELOG.md lacks {heading.strip()!r}")
            changelog = changelog.replace(
                heading, f"{heading}- **{new}** {stub}\n\n", 1
            )
    changes[root / "CHANGELOG.md"] = changelog
    return changes


def pinned_hashes(root):
    build = root / "build/windows"
    lines = [
        f"{hashlib.sha256((build / name).read_bytes()).hexdigest()}  {name}"
        for name in DLLS
    ]
    return {root / "tools/ci/dll.sha256": "\n".join(lines) + "\n"}


def main(argv=None):
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("--version", help="new version, major.minor.patch")
    parser.add_argument(
        "--pin-dlls", action="store_true", help="pin build/windows DLLs"
    )
    parser.add_argument("--apply", action="store_true", help="write the changes")
    args = parser.parse_args(argv)
    if not args.version and not args.pin_dlls:
        parser.error("expected --version, --pin-dlls or both")
    if args.version and not re.fullmatch(r"\d+\.\d+\.\d+", args.version):
        parser.error("--version must be major.minor.patch")
    changes = {}
    if args.version:
        changes.update(versioned_files(ROOT, args.version))
    if args.pin_dlls:
        changes.update(pinned_hashes(ROOT))
    for path, text in changes.items():
        if path.read_text(encoding="utf-8") == text:
            continue
        print(
            ("Write " if args.apply else "Would write ") + str(path.relative_to(ROOT))
        )
        if args.apply:
            path.write_text(text, encoding="utf-8")
    if not args.apply:
        print("Preview only. Add --apply to write.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
