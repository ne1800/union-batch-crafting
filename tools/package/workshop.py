"""Build and verify the Steam Workshop content directory for Gothic II."""

import argparse
import hashlib
from pathlib import Path
import runpy
import tempfile

from . import vdf
from ..release.release import pinned_dll

ROOT = Path(__file__).resolve().parents[2]
PLAYER = runpy.run_path(str(ROOT / "tools/package/package-player-WRITES.py"))
DLL = PLAYER["DLL"]
MEMBER = f"SYSTEM/AUTORUN/{DLL.upper()}"
NOTICES = ("LICENSE", "THIRD-PARTY-NOTICES")
ARCHIVE = Path("dist/workshop/content/Data/UnionBatchCrafting.vdf")


def notices(root):
    return {
        f"DOCS/UNIONBATCHCRAFTING/{name}": (root / name).read_bytes()
        for name in NOTICES
    }


def checked_player(root, data):
    digest = hashlib.sha256(data).hexdigest()
    if digest != pinned_dll(root):
        raise ValueError(
            "Workshop DLL differs from the player pin in tools/ci/dll.sha256"
        )
    PLAYER["validate_dll"](data, (root / "VERSION").read_text().strip())
    return digest


def check_content_tree(target):
    content = target.parent.parent
    if target.resolve() != target:
        raise ValueError("Linked Workshop output path")
    for path in content.rglob("*"):
        if path == target.parent and path.is_dir():
            continue
        if path == target and path.is_file():
            continue
        raise ValueError(f"Unexpected Workshop content: {path.relative_to(content)}")


def verify(root, data):
    (comment, stamp, size), files = vdf.read_volume(data)
    if (comment, stamp, size) != (
        vdf.COMMENT,
        vdf.TIMESTAMP,
        sum(map(len, files.values())),
    ):
        raise ValueError("Unexpected Workshop volume header")
    if MEMBER not in files:
        raise ValueError("Missing autorun DLL")
    digest = checked_player(root, files[MEMBER])
    if files != {MEMBER: files[MEMBER], **notices(root)}:
        raise ValueError("Unexpected Workshop members or licence text")
    if vdf.pack(files) != data:
        raise ValueError("Unexpected bytes in the Workshop volume")
    return digest


def build(root):
    target = root / ARCHIVE
    check_content_tree(target)
    dll = (root / "build/windows/player" / DLL).read_bytes()
    volume = vdf.pack({MEMBER: dll, **notices(root)})
    verify(root, volume)
    target.parent.mkdir(parents=True, exist_ok=True)
    # Stage outside content so a simultaneous reader only sees a complete volume.
    with tempfile.TemporaryDirectory(dir=target.parents[2]) as staging:
        temp = Path(staging) / target.name
        temp.write_bytes(volume)
        temp.chmod(0o644)
        temp.replace(target)
    return target


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="action", required=True)
    commands.add_parser("build").add_argument("--apply", action="store_true")
    commands.add_parser("verify")
    args = parser.parse_args()
    if args.action == "build":
        if args.apply:
            print(build(ROOT).relative_to(ROOT))
        else:
            print(f"Preview: write {ARCHIVE} with --apply")
    else:
        target = ROOT / ARCHIVE
        check_content_tree(target)
        print(f"{verify(ROOT, target.read_bytes())}  {MEMBER}")


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError) as error:
        raise SystemExit(str(error))
