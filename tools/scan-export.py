"""Check export paths, tracked contents and all Git objects, including unreachable objects."""

import re
import runpy
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


def findings(name, data, patterns):
    try:
        body = data.decode("utf-8")
    except UnicodeError:
        return ["binary"]
    text = unquote(name + "\n" + body).replace(r"\.", ".")
    hits = [
        label
        for label, pattern in patterns.items()
        if pattern.search(
            text.replace(IDENTITY, "") if label == "email address" else text
        )
    ]
    if "\0" in text or any(ord(c) < 32 and c not in "\n\r\t" for c in body):
        hits.append("binary")
    return hits


def git(root, *args):
    return subprocess.check_output(["git", "-C", root, *args])


def scan(root, patterns):
    def check(name, data, object_id):
        if issues := findings(name, data, patterns):
            raise ValueError(f"Export scan failed ({object_id}): {', '.join(issues)}")

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
            check("", git(root, "cat-file", kind, oid), oid)
    print(f"Export scan passed: {len(files)} files, {len(objects)} Git objects")


if __name__ == "__main__":
    root = Path(__file__).resolve().parents[1]
    exporter = root / "tools/public-export-WRITES.py"
    if exporter.exists():
        private = runpy.run_path(str(exporter))
        scan(private["TARGET"], private["private_patterns"](root))
    else:
        scan(
            root,
            {label: re.compile(rule, re.I | re.M) for label, rule in RULES.items()},
        )
