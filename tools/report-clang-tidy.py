"""Report diagnostics for production translation units in the Windows build."""

import argparse
from concurrent.futures import ThreadPoolExecutor
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
BASELINE = ROOT / "tools/clang-tidy-baseline.json"
DIAGNOSTIC = re.compile(r"^(.+?):(\d+):(\d+): warning: (.+) \[([^\]]+)\]$", re.M)


def inspect_file(build, source):
    run = subprocess.run(
        ["clang-tidy", "--quiet", "-p", build, source],
        capture_output=True,
        text=True,
        check=False,
    )
    if run.returncode:
        raise SystemExit(run.stdout + run.stderr)
    issues = set()
    for file, line, column, message, check in DIAGNOSTIC.findall(run.stdout):
        path = Path(file).resolve()
        if path.is_relative_to(ROOT):
            issues.add(f"{path.relative_to(ROOT)}:{line}:{column}: {message} [{check}]")
    return issues


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, default=ROOT / "build/windows")
    args = parser.parse_args()
    database = args.build / "compile_commands.json"
    if not database.is_file():
        parser.exit(
            2, "Missing compile_commands.json. Run mise run build:windows first.\n"
        )
    entries = json.loads(database.read_text())
    paths = {Path(row["directory"], row["file"]).resolve() for row in entries}
    sources = sorted(
        path
        for path in paths
        if path.suffix in {".c", ".cc", ".cpp", ".cxx"}
        and path.is_relative_to(ROOT / "src")
    )
    if not sources:
        parser.exit(
            2, "No production translation units found in compile_commands.json.\n"
        )
    with ThreadPoolExecutor(max_workers=3) as executor:
        results = executor.map(lambda source: inspect_file(args.build, source), sources)
        current = set().union(*results)
    baseline = set(json.loads(BASELINE.read_text()))
    for issue in sorted(current - baseline):
        print(f"NEW {issue}")
    for issue in sorted(baseline - current):
        print(f"RESOLVED {issue}")
    print(
        f"{len(sources)} translation units, {len(current)} diagnostics, "
        f"{len(current - baseline)} new, {len(baseline - current)} resolved"
    )


if __name__ == "__main__":
    main()
