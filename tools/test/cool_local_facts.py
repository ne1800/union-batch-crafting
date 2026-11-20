"""Resolve local files for the cooling composite. Read-only.

The C++ hosts check all PE and DAT hashes. This does not establish the identities
of loaded modules. The renderer belongs only to the existing RAM fixture.
"""

from pathlib import Path


def local_files(game_exe: Path, archive: Path) -> list[Path]:
    # Retain the selected Steam and Workshop roots before resolving
    # paths. Individual installation files may link to another drive.
    system = game_exe.absolute().parent
    game = system.parent
    workshop = archive.absolute().parents[3]
    files = [
        game_exe,
        archive,
        game / "Data/$Templates$/ZBUGFIXES.DLL",
        workshop / "2787458679/System/Shw32.dll",
        system / "msvcr100.dll",
        game / "Data/$Templates$/zParserExtender.dll",
        system / "gd3d11/bin/g2a_avx2.dll",
    ]
    result = [path.resolve(strict=True) for path in files]
    if any(not path.is_file() for path in result):
        raise ValueError("Kühl-Komposit benötigt sieben reguläre lokale Dateien")
    return result
