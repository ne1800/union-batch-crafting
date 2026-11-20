"""Read only the world ID from a narrowly bound oCSavegameInfo ASCII archive.

No file operations. Return no title, user or character data. The observation
describes the save on disk, without establishing which world is already loaded.
"""

import re


def saved_world(data):
    def require(ok):
        if not ok:
            raise ValueError("SAVEINFO-Format oder Weltkennung nicht eindeutig bekannt")

    require(isinstance(data, bytes) and 0 < len(data) <= 65536 and b"\0" not in data)
    lines = [line.strip() for line in data.decode("cp1252").splitlines()]
    require(lines[:4] == ["ZenGin Archive", "ver 1", "zCArchiverGeneric", "ASCII"])
    require("END" in lines[4:])
    end = lines.index("END", 4)
    header = {}
    for line in lines[4:end]:
        require(" " in line and len(line) <= 2048)
        key, value = line.split(" ", 1)
        require(key in ("saveGame", "date", "user") and key not in header)
        header[key] = value
    require(header.get("saveGame") == "0")
    tail = [line for line in lines[end + 1 :] if line]
    require(
        tail[:3] == ["objects 1", "END", "[% oCSavegameInfo 0 0]"]
        and tail[-1:] == ["[]"]
    )
    kinds = {
        "Title": "string",
        "WorldName": "string",
        "TimeDay": "int",
        "TimeHour": "int",
        "TimeMin": "int",
        "SaveDate": "string",
        "VersionMajor": "int",
        "VersionMinor": "int",
        "PlayTimeSeconds": "int",
        "VersionPoint": "int",
        "VersionInt": "int",
        "VersionAppName": "string",
    }
    seen = set()
    world = None
    for line in tail[3:-1]:
        match = re.fullmatch(r"([A-Za-z][A-Za-z0-9_]*)=([A-Za-z]+):(.*)", line)
        require(match is not None and len(line) <= 2048)
        key, kind, value = match.groups()
        require(key in kinds and key not in seen and kinds[key] == kind)
        seen.add(key)
        if kind == "int":
            require(re.fullmatch(r"-?[0-9]{1,11}", value) is not None)
            require(-(1 << 31) <= int(value) < (1 << 31))
        if key == "WorldName":
            require(re.fullmatch(r"[A-Z][A-Z0-9_]{0,63}(?:\.ZEN)?", value) is not None)
            world = value.removesuffix(".ZEN")
    require(seen == kinds.keys() and world is not None)
    return world
