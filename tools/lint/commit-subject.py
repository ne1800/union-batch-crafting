"""Validate the subject of a Conventional Commit. Review checks its language."""

import re
import sys
from pathlib import Path

SUBJECT = re.compile(
    r"(?:build|chore|ci|docs|feat|fix|perf|refactor|revert|style|test)"
    r"(?:\([a-z0-9][a-z0-9._/-]*\))?!?: \S(?:.*\S)?"
)


def valid(subject):
    return SUBJECT.fullmatch(subject) is not None


if __name__ == "__main__":
    if sys.argv[1:] == ["--stdin"]:
        for line in sys.stdin:
            if not valid(line.rstrip("\n")):
                sys.exit(f"Invalid Conventional Commit subject: {line.rstrip()}")
        sys.exit(0)
    with Path(sys.argv[1]).open(encoding="utf-8") as message:
        if not valid(message.readline().rstrip("\n")):
            sys.exit("Use type(scope): English subject, with optional scope and !")
