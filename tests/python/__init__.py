"""Discover each area with the original, unqualified unittest module names."""

from pathlib import Path


def load_tests(loader, suite, pattern):
    root = Path(__file__).resolve().parent
    for directory in sorted({path.parent for path in root.rglob(pattern)}):
        suite.addTests(loader.discover(str(directory), pattern, str(directory)))
    return suite
