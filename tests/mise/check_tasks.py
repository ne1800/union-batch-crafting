"""Check the checkout's mise task list and references in maintained guides."""

from difflib import unified_diff
import json
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
EXPECTED = Path(__file__).with_name("tasks.json")
# Match a task as a token, excluding script filenames and qualified task names.
RETIRED = re.compile(
    r"(?<![\w:./-])(?:package-(?:(?:batch|crafting|menu|player|recipes|selection|station)-)?WRITES"
    r"|export-public-WRITES)(?![\w:./-])"
    r"|\bmise\s+(?:(?:run|r)\s+)?(?:lint|state)(?![\w:./-])"
    r"|`(?:lint|state)`"
)


def main(extra_guides):
    expected = json.loads(EXPECTED.read_text())
    actual = sorted(
        subprocess.check_output(
            ["mise", "tasks", "--no-global", "--name-only"], cwd=ROOT, text=True
        ).splitlines()
    )
    errors = list(unified_diff(expected, actual, "expected", "mise tasks", lineterm=""))
    guides = set(ROOT.glob("*.md"))
    for folder in ("docs", "publication", "publication/docs", "tools/world"):
        guides.update((ROOT / folder).glob("*.md"))
    guides.update((ROOT / ".agents/skills").glob("*/SKILL.md"))
    guides.update(map(Path, extra_guides))
    for path in sorted(guides):
        for number, line in enumerate(path.read_text().splitlines(), 1):
            if match := RETIRED.search(line):
                errors.append(f"{path}:{number}: retired task {match[0]}")
    if errors:
        raise SystemExit("\n".join(errors))
    print(f"Verified {len(expected)} mise tasks and {len(guides)} guides.")


if __name__ == "__main__":
    main(sys.argv[1:])
