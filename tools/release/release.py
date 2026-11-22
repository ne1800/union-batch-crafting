#!/usr/bin/env python3
"""Check the release metadata of VERSION and print release notes. Read-only."""

import argparse
import hashlib
from pathlib import Path
import re
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[2]
DLL_MEMBER = "system/autorun/UnionBatchCraftingExtended.dll"
PLAYER_DLL = "player/UnionBatchCraftingExtended.dll"
HEADINGS = {"English": "## English", "Deutsch": "## Deutsch"}
GUIDES = {"INSTALL.md": "for players", "INSTALL.de.md": "für Spieler"}
PLACEHOLDER = "TODO"
ARCHIVE = re.compile(r"union-batch-crafting-player-(\d+\.\d+\.\d+)\.zip")
TESTED_DLL = re.compile(
    r"\| Player DLL / Spieler-DLL \(([^)]*)\) \| `([0-9a-f]{64})` \|"
)


def version(root):
    text = (root / "VERSION").read_text(encoding="utf-8").strip()
    if not re.fullmatch(r"\d+\.\d+\.\d+", text):
        raise ValueError(f"VERSION must be major.minor.patch, got {text!r}")
    return text


def archive_name(release):
    return f"union-batch-crafting-player-{release}.zip"


def changelog_entry(root, release, language):
    """Return the text of one changelog entry, or None when it is missing."""
    lines = (root / "CHANGELOG.md").read_text(encoding="utf-8").splitlines()
    if HEADINGS[language] not in lines:
        return None
    prefix = f"- **{release}** "
    entry = None
    for line in lines[lines.index(HEADINGS[language]) + 1 :]:
        if line.startswith("## "):
            break
        if entry is not None:
            if not line.strip() or line.startswith("- "):
                break
            entry.append(line.strip())
        elif line.startswith(prefix):
            entry = [line.removeprefix(prefix).strip()]
    return " ".join(entry) if entry else None


def tested_dll(root, release):
    """Return the player DLL hash that TESTED-WITH.md records for a version."""
    text = (root / "TESTED-WITH.md").read_text(encoding="utf-8")
    for match in TESTED_DLL.finditer(text):
        if release in (item.strip() for item in match[1].split(",")):
            return match[2]
    return None


def pinned_dll(root):
    for line in (root / "tools/ci/dll.sha256").read_text(encoding="utf-8").splitlines():
        digest, _, name = line.partition("  ")
        if name == PLAYER_DLL:
            return digest
    raise ValueError(f"tools/ci/dll.sha256 lacks {PLAYER_DLL}")


def consistency_errors(root):
    """Every versioned file names VERSION, and both changelogs describe it."""
    release = version(root)
    errors = []
    for name, audience in GUIDES.items():
        text = (root / name).read_text(encoding="utf-8")
        if f"# Union Batch Crafting {release} {audience}\n" not in text:
            errors.append(f"{name}: the title must name {release}")
        if {match[1] for match in ARCHIVE.finditer(text)} != {release}:
            errors.append(f"{name}: the player ZIP must be {archive_name(release)}")
    notices = (root / "THIRD-PARTY-NOTICES").read_text(encoding="utf-8")
    if notices.partition("\n")[0] != f"Union Batch Crafting {release}":
        errors.append(f"THIRD-PARTY-NOTICES: the first line must name {release}")
    for language in HEADINGS:
        entry = changelog_entry(root, release, language)
        if not entry or PLACEHOLDER in entry:
            errors.append(f"CHANGELOG.md: describe {release} in the {language} section")
    return errors


def evidence_errors(root):
    """The pinned player DLL is the one recorded from the in-game test."""
    release = version(root)
    tested = tested_dll(root, release)
    if tested is None:
        return [f"TESTED-WITH.md: record the player DLL tested in game for {release}"]
    pinned = pinned_dll(root)
    if pinned != tested:
        return [
            f"tools/ci/dll.sha256 pins player DLL {pinned[:12]}, "
            f"but the in-game test used {tested[:12]}"
        ]
    return []


def notes(root, archive, repository):
    """Verify the player ZIP against the in-game test and return release notes."""
    release = version(root)
    errors = consistency_errors(root) + evidence_errors(root)
    if archive.name != archive_name(release):
        errors.append(f"Expected {archive_name(release)}, got {archive.name}")
    with zipfile.ZipFile(archive) as content:
        dll = hashlib.sha256(content.read(DLL_MEMBER)).hexdigest()
    if dll != tested_dll(root, release):
        errors.append("The DLL in the player ZIP is not the one tested in game")
    if errors:
        raise ValueError("\n".join(errors))
    digest = hashlib.sha256(archive.read_bytes()).hexdigest()
    base = f"https://github.com/{repository}/blob/v{release}"
    return "\n".join(
        (
            changelog_entry(root, release, "English"),
            "",
            changelog_entry(root, release, "Deutsch"),
            "",
            "| File / Datei | SHA-256 |",
            "| --- | --- |",
            f"| `{archive.name}` | `{digest}` |",
            f"| `{DLL_MEMBER}` | `{dll}` |",
            "",
            f"[Install / Installation]({base}/INSTALL.md) · "
            f"[Tested with / Getestet mit]({base}/TESTED-WITH.md) · "
            f"[Changelog]({base}/CHANGELOG.md)",
            "",
        )
    )


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    check = commands.add_parser("check", help="check the files that name VERSION")
    check.add_argument(
        "--evidence",
        action="store_true",
        help="also require the in-game test of the pinned player DLL",
    )
    release_notes = commands.add_parser("notes", help="print notes for the player ZIP")
    release_notes.add_argument("archive", type=Path)
    release_notes.add_argument("--repository", required=True, help="owner/name")
    args = parser.parse_args(argv)
    try:
        if args.command == "notes":
            sys.stdout.write(notes(ROOT, args.archive, args.repository))
            return 0
        errors = consistency_errors(ROOT)
        if args.evidence:
            errors += evidence_errors(ROOT)
    except (OSError, ValueError, KeyError, zipfile.BadZipFile) as error:
        print(error, file=sys.stderr)
        return 1
    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1
    print(f"Release metadata for {version(ROOT)} is consistent.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
