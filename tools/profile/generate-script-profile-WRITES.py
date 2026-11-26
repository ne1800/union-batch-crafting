#!/usr/bin/env python3
"""Generate a reference script profile header for the tolerant recipe check.

Reads one reference GOTHIC.DAT (directly or from a .mod/.vdf volume), runs
the plugin verifiers through record_catalog_reads.cpp against a temporary
parser image and derives names, UBCF1 function records, data records and
recipe groups. Without --apply it only prints the plan and whether the
header would change. WRITES include/ubc/catalogs/generated/<campaign>.hpp
only with --apply. Temporary files go to the system temp directory and are
removed afterwards. The DAT is only read.
"""

from __future__ import annotations

import argparse
import difflib
import hashlib
import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(ROOT / "tools/package"))

import catalog_literals  # noqa: E402
import parser_image  # noqa: E402
import script_profile  # noqa: E402
from datfile import DatFile  # noqa: E402

CAMPAIGNS = {
    "marcello": {
        "title": "L'Hiver Marcello 2.0.4 (German)",
        "namespace": "marcello_204",
        "header": "marcello_204_fingerprints.hpp",
        "size": 9327865,
        "sha256": "71a7875aa24029ea6367c69d9f8c2d6e8c18a92aae002cc292bd92e5c907fbb4",
        "runtime_names": True,
        "parser": 0x00AB40C0,
    },
    "dndr": {
        "title": "Gothic II Night of the Raven 2.6 (German)",
        "namespace": "dndr_26",
        "header": "dndr_26_fingerprints.hpp",
        "size": 7587412,
        "sha256": "b9e18ac81e08f9eac5942e23adff180f8ca239730eb4c4937b87a4f5bf7beda8",
        "runtime_names": False,
        "parser": 0x00AB40C0,
    },
}
OUTPUT = ROOT / "include/ubc/catalogs/generated"


def read_dat(arguments) -> bytes:
    if arguments.dat:
        return Path(arguments.dat).read_bytes()
    import vdf

    _, files = vdf.read_volume(Path(arguments.mod).read_bytes())
    found = [path for path in files if path.endswith("/_COMPILED/GOTHIC.DAT")]
    if len(found) != 1:
        raise SystemExit(
            f"{arguments.mod}: expected one compiled GOTHIC.DAT, found {len(found)}"
        )
    return files[found[0]]


def recorder(arguments, work: Path) -> Path:
    if arguments.recorder:
        return Path(arguments.recorder)
    binary = work / "record_catalog_reads"
    command = [
        arguments.cxx,
        "-std=c++17",
        "-O1",
        "-I",
        str(ROOT / "include"),
        str(HERE / "record_catalog_reads.cpp"),
        "-o",
        str(binary),
    ]
    subprocess.run(command, check=True)
    return binary


def record(arguments, dat: DatFile, campaign: str, work: Path) -> list[dict]:
    names = (
        parser_image.runtime_anonymous_names(dat, ROOT / "include")
        if CAMPAIGNS[campaign]["runtime_names"]
        else {}
    )
    image = work / f"{campaign}.img"
    parser_image.write(image, parser_image.build(dat, names))
    output = subprocess.run(
        [str(recorder(arguments, work)), campaign, str(image)],
        check=True,
        capture_output=True,
        text=True,
    ).stdout
    return json.loads(output)["recipes"]


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Generate the UBCF1 reference profile header of one campaign. "
        "Read-only plan by default; WRITES include/ubc/catalogs/generated/ only with --apply."
    )
    parser.add_argument("--campaign", choices=sorted(CAMPAIGNS), required=True)
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--dat", help="reference GOTHIC.DAT (read only)")
    source.add_argument(
        "--mod", help=".mod or .vdf volume holding the compiled GOTHIC.DAT"
    )
    parser.add_argument("--apply", action="store_true", help="write the header")
    parser.add_argument("--output-dir", type=Path, default=OUTPUT)
    parser.add_argument("--recorder", help="prebuilt record_catalog_reads binary")
    parser.add_argument("--cxx", default=os.environ.get("CXX", "c++"))
    arguments = parser.parse_args(argv)

    config = CAMPAIGNS[arguments.campaign]
    data = read_dat(arguments)
    digest = hashlib.sha256(data).hexdigest()
    if len(data) != config["size"] or digest != config["sha256"]:
        raise SystemExit(
            f"not the {arguments.campaign} reference script: {len(data)} bytes, "
            f"SHA-256 {digest[:8]}..."
        )
    dat = DatFile.parse(data)
    with tempfile.TemporaryDirectory(prefix="ubc-profile-") as directory:
        recipes = record(arguments, dat, arguments.campaign, Path(directory))
    literals = catalog_literals.scan(ROOT / "include")
    profile = script_profile.Builder(arguments.campaign, dat, recipes, literals).build()
    text = script_profile.render(profile, config)
    target = arguments.output_dir / config["header"]
    old = target.read_text(encoding="ascii") if target.exists() else ""
    summary = dict(profile.summary, bytes=len(text.encode()))
    print(f"{arguments.campaign}: {json.dumps(summary, sort_keys=True)}")
    if old == text:
        print(
            f"{target.relative_to(ROOT) if target.is_relative_to(ROOT) else target}: unchanged"
        )
        return 0
    changed = sum(
        1
        for line in difflib.unified_diff(old.splitlines(), text.splitlines(), n=0)
        if line[:1] in "+-" and line[:3] not in ("+++", "---")
    )
    print(f"{target.name}: {changed} changed lines")
    if not arguments.apply:
        print("plan only; rerun with --apply to write")
        return 0
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(text, encoding="ascii")
    print(f"wrote {target.name}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
