"""The uncompressed Gothic II VDFS volume format used by Workshop addons."""

import re
import struct

SIGNATURE = b"PSVDSC_V2.00\n\r\n\r"
COMMENT = b"Union Batch Crafting".ljust(256, b"\x1a")
TIMESTAMP = (20 << 25) | (1 << 21) | (1 << 16)  # DOS FAT 2000-01-01 00:00:00
HEADER = struct.Struct("<256s16s6I")
ENTRY = struct.Struct("<64s4I")
DIRECTORY = 0x80000000
LAST = 0x40000000


def pack(files):
    """Place siblings contiguously. Directories point to table indices, files to bytes."""
    root = {}
    for path, data in files.items():
        parts = path.split("/")
        if not all(re.fullmatch(r"[A-Z0-9_-][A-Z0-9_.-]{0,63}", p) for p in parts):
            raise ValueError(f"Invalid VDF member name: {path}")
        group = root
        for part in parts[:-1]:
            group = group.setdefault(part, {})
            if not isinstance(group, dict):
                raise ValueError(f"File used as a directory: {path}")
        if parts[-1] in group:
            raise ValueError(f"Conflicting VDF member: {path}")
        group[parts[-1]] = data
    if not root:
        raise ValueError("Empty VDF")

    entries = []
    payload = bytearray()

    def directory(children):
        start = len(entries)
        entries.extend([None] * len(children))
        for index, name in enumerate(sorted(children), start):
            value = children[name]
            flags = LAST if index + 1 == start + len(children) else 0
            if isinstance(value, dict):
                position = directory(value)
                row = (position, 0, flags | DIRECTORY, 0)
            else:
                row = (len(payload), len(value), flags, 32)
                payload.extend(value)
            entries[index] = (name.encode("ascii").ljust(64, b" "), *row)
        return start

    directory(root)
    header = HEADER.pack(
        COMMENT,
        SIGNATURE,
        len(entries),
        len(files),
        TIMESTAMP,
        len(payload),
        HEADER.size,
        80,
    )
    data_start = len(header) + len(entries) * ENTRY.size
    table = bytearray()
    for name, position, size, flags, attributes in entries:
        if not flags & DIRECTORY:
            position += data_start
        table.extend(ENTRY.pack(name, position, size, flags, attributes))
    return header + table + payload


def read_volume(data):
    """Read a classic uncompressed, catalogue-first volume without extracting to disk."""
    if len(data) < HEADER.size:
        raise ValueError("Truncated VDF header")
    comment, signature, count, file_count, stamp, total, catalogue, version = (
        HEADER.unpack_from(data)
    )
    if signature not in (SIGNATURE, b"PSVDSC_V2.00\x1a\x1a\x1a\x1a") or version != 80:
        raise ValueError("Expected an uncompressed Gothic VDFS volume")
    data_start = catalogue + count * ENTRY.size
    if catalogue < HEADER.size or data_start > len(data) or not count:
        raise ValueError("Invalid VDF catalogue")

    files, paths, visited = {}, set(), set()
    pending = [("", 0)]
    while pending:
        prefix, index = pending.pop()
        while True:
            if index >= count or index in visited:
                raise ValueError("Invalid VDF directory link or terminator")
            visited.add(index)
            raw, offset, size, flags, _ = ENTRY.unpack_from(
                data, catalogue + index * ENTRY.size
            )
            name = raw.rstrip(b" \x00").decode("ascii")
            if not name or name in (".", "..") or any(c in name for c in "/\\\x00"):
                raise ValueError("Invalid VDF path")
            path = prefix + name.upper()
            if path in paths or flags & ~(DIRECTORY | LAST):
                raise ValueError("Ambiguous VDF catalogue entry")
            paths.add(path)
            if flags & DIRECTORY:
                pending.append((path + "/", offset))
            else:
                if offset < data_start or offset + size > len(data):
                    raise ValueError("File outside VDF payload")
                files[path] = data[offset : offset + size]
            if flags & LAST:
                break
            index += 1
    if len(visited) != count or len(files) != file_count:
        raise ValueError("VDF catalogue count mismatch")
    return (comment, stamp, total), files
