"""Check repository paths, tracked contents and all Git objects, including unreachable objects."""

import re
import json
import os
import subprocess
from pathlib import Path
from urllib.parse import unquote

IDENTITY = "34651555+ne1800@users.noreply.github.com"
RULES = {
    "user directory": r"(?:/(?:home|Users)/|[a-z]:[/\\]+users[/\\]+)[\w.-]+",
    "email address": r"[a-z0-9._%+-]+@[a-z0-9.-]+\.[a-z]{2,}",
    "assigned credential": r"""(?<![.\w])(?:token|secret|passphrase|session[_-]?token|aws[_-]?secret[_-]?access[_-]?key)["']?\s*[:=]\s*["'](?!\\)[^"'\n]+["']|(?:^\s*|[?&])(?:token|secret|api[_-]?key)=[a-z0-9_+/=-]{12,}""",
    "Steam account": r"\b(?:7656119\d{10}|STEAM_[0-5]:[01]:\d+)\b|\[U:1:\d+\]|(?:userdata|Steam Controller Configs)[/\\]+\d+|steamcommunity\.com/(?:id|profiles)/",
    "access credential": r"(?:gh[pousr]_|github_pat_|glpat-|xox[baprs]-|sk-(?:proj-|live[_-])?)[A-Za-z0-9_-]{12,}|AKIA[A-Z0-9]{16}|-----BEGIN [A-Z ]*PRIVATE KEY-----|ssh-(?:ed25519|rsa) [A-Za-z0-9+/]{20,}",
    "account data": r"(?:(?<![.\w])(?:password|api[_-]?key|access[_-]?token|client[_-]?secret|sessionid|cookie|steamLoginSecure)[\"']?\s*[:=]\s*[\"']?[^\s\"'{};,]{6,}|Authorization:\s*(?:Bearer|Basic)\s+\S+|https?://[^/\s:@]+:[^/\s@]+@|eyJ[A-Za-z0-9_-]{12,}\.[A-Za-z0-9_-]+\.[A-Za-z0-9_-]+)",
}


# README screenshots are the only binary content. Each must be one still WebP
# image without an extended header, so it cannot carry EXIF, XMP or ICC data.
IMAGE_PATH = re.compile(r"docs/images/[a-z0-9-]+[.]webp")
IMAGE_LIMIT = 300_000


def plain_webp(data):
    if (
        not 20 <= len(data) <= IMAGE_LIMIT
        or data[:4] != b"RIFF"
        or data[8:12] != b"WEBP"
    ):
        return False
    size = int.from_bytes(data[16:20], "little")
    return (
        int.from_bytes(data[4:8], "little") == len(data) - 8
        and data[12:16] in (b"VP8 ", b"VP8L")
        and 20 + size + (size & 1) == len(data)
    )


def findings(name, data, patterns, *, commit=False):
    # Git objects have no name here; the tree scan checks their paths separately.
    if plain_webp(data) and (not name or IMAGE_PATH.fullmatch(name)):
        return findings(name, b"", patterns)
    try:
        body = data.decode("utf-8")
    except UnicodeError:
        return ["binary"]
    text = unquote(name + "\n" + body).replace(r"\.", ".")
    mail_text = text.replace(IDENTITY, "")
    if commit:
        # A fetched Renovate onboarding ref may contain GitHub web-flow commits.
        # Only the exact public service mailbox in a Git committer header is OK.
        headers, separator, message = body.partition("\n\n")
        headers = re.sub(
            r"(?m)^committer GitHub <noreply[@]github[.]com>(?= [0-9]+ [+-][0-9]{4}$)",
            "committer GitHub <web-flow>",
            headers,
        )
        mail_text = (
            unquote(name + "\n" + headers + separator + message)
            .replace(r"\.", ".")
            .replace(IDENTITY, "")
        )
    hits = [
        label
        for label, pattern in patterns.items()
        if pattern.search(mail_text if label == "email address" else text)
    ]
    if "\0" in text or any(ord(c) < 32 and c not in "\n\r\t" for c in body):
        hits.append("binary")
    return hits


def git(root, *args):
    return subprocess.check_output(
        ["git", "--no-replace-objects", "-C", root, *args], stderr=subprocess.DEVNULL
    )


def scan(root, patterns):
    def check(name, data, object_id, *, commit=False):
        if issues := findings(name, data, patterns, commit=commit):
            raise ValueError(f"Leak scan failed ({object_id}): {', '.join(issues)}")

    files = git(root, "ls-files", "-z").decode().split("\0")[:-1]
    for name in files:
        path = root / name
        if path.is_symlink() or not path.is_file():
            raise ValueError("Expected a tracked ordinary file")
        check(name, path.read_bytes(), "working tree")
    objects = (
        git(
            root,
            "cat-file",
            "--batch-all-objects",
            "--batch-check=%(objectname) %(objecttype)",
        )
        .decode()
        .splitlines()
    )
    for row in objects:
        oid, kind = row.split()
        if kind == "tree":
            # Check joined path components, including directories and orphan trees.
            names = git(root, "ls-tree", "-r", "-t", "-z", "--name-only", oid)
            for raw in names.split(b"\0")[:-1]:
                check(raw.decode(), b"", oid)
        else:
            check("", git(root, "cat-file", kind, oid), oid, commit=kind == "commit")
    print(f"Leak scan passed: {len(files)} files, {len(objects)} Git objects")


def load_patterns(root, additional=()):
    """Read JSON arrays of regexes. A configured but unreadable file is an error."""
    configured = os.environ.get("UBC_LEAK_PATTERNS")
    if configured is None:
        result = subprocess.run(
            ["git", "-C", root, "config", "--local", "--get", "ubc.leakPatterns"],
            capture_output=True,
            text=True,
        )
        if result.returncode == 0:
            configured = result.stdout.rstrip("\n")
        elif result.returncode != 1:
            raise ValueError("Cannot read local Git configuration")
    paths = list(additional)
    if configured is not None:
        paths.append(root / configured)
    rules = {label: re.compile(rule, re.I | re.M) for label, rule in RULES.items()}
    for path in paths:
        try:
            extra = json.loads(path.read_text(encoding="utf-8"))
            if not isinstance(extra, list) or not all(
                isinstance(item, str) and item for item in extra
            ):
                raise ValueError
            for pattern in extra:
                rules[f"local pattern {len(rules)}"] = re.compile(pattern, re.I | re.M)
        except (OSError, ValueError, re.error):
            raise ValueError("Cannot load local scan patterns") from None
    return rules


if __name__ == "__main__":
    try:
        root = Path(git(Path.cwd(), "rev-parse", "--show-toplevel").decode().strip())
        scan(root, load_patterns(root))
    except (UnicodeError, OSError, subprocess.SubprocessError):
        raise SystemExit("Cannot scan repository") from None
    except ValueError as exc:
        raise SystemExit(str(exc)) from None
