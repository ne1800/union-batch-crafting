"""Run an installed formatter only if its version matches the repository pin."""

import os
import re
import subprocess
import sys

tool, version, *args = sys.argv[1:]
try:
    banner = subprocess.check_output([tool, "--version"], text=True)
except FileNotFoundError:
    sys.exit(f"{tool} {version} is required")
if not re.search(r"(?<![\w.])" + re.escape(version) + r"(?![\w.])", banner):
    sys.exit(f"Expected {tool} {version}, found {banner.strip()}")
os.execvp(tool, [tool, *args])
