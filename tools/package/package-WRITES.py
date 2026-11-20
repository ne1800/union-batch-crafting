#!/usr/bin/env python3
"""Write the deterministic diagnostic package to dist with --apply."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct
import zipfile

ROOT = Path(__file__).resolve().parents[2]
VERSION = "0.0.1"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--apply", action="store_true", help="Create the diagnostic package in dist"
    )
    args = parser.parse_args()
    if not args.apply:
        print(f"Plan: Diagnosepaket {VERSION} unter dist; Schreiben nur mit --apply.")
        return
    out = ROOT / "dist"
    archive = out / f"union-batch-crafting-probe-{VERSION}.zip"
    if out.resolve() != out or archive.is_symlink():
        raise SystemExit("Unsicheres Paket-Ausgabeverzeichnis.")
    dll = (ROOT / "build/windows/UnionBatchCraftingProbe.dll").read_bytes()
    if len(dll) < 64:
        raise SystemExit("DLL ist für einen PE-Kopf zu kurz.")
    pe = struct.unpack_from("<I", dll, 0x3C)[0]
    if pe < 64 or pe > len(dll) - 26:
        raise SystemExit("Ungültige Position des PE-Kopfs.")
    if (
        dll[:2] != b"MZ"
        or dll[pe : pe + 4] != b"PE\0\0"
        or struct.unpack_from("<H", dll, pe + 4)[0] != 0x14C
    ):
        raise SystemExit("Build ist kein Windows-x86-PE.")
    if not struct.unpack_from("<H", dll, pe + 22)[0] & 0x2000:
        raise SystemExit("Build ist keine DLL.")
    if struct.unpack_from("<H", dll, pe + 24)[0] != 0x10B:
        raise SystemExit("Build ist kein PE32-Programm.")
    digest = hashlib.sha256(dll).hexdigest()
    instruction = (
        "Union Batch Crafting – Diagnoseprototyp 0.0.1\n\n"
        "Dies ist noch keine Handwerksmod. Keine Rezepte, Menüs oder Hooks werden installiert.\n"
        "Nur für einen ausdrücklich freigegebenen Ladetest mit Gothic II DNdR und Union.\n"
        "Nach Sicherung eventuell vorhandener gleichnamiger Dateien die DLL unter\n"
        "Gothic II/System/Autorun/ ablegen. Keine anderen Moddateien ersetzen.\n"
        "Beim Laden unter passendem vorläufigem PE-Kopffilter entsteht daneben\n"
        "UnionBatchCraftingProbe.log. Diese Datei belegt nur das Laden der DLL.\n"
        "Sie beweist weder Hauptmenü noch Spielwelt- oder Controllerfunktion.\n"
        "Die vier erwarteten Zeilen lauten:\n"
        "probe_version=1\ntarget_header_match=true\ngameplay_hooks=false\nscript_calls=false\n\n"
        "Für den Rückbau das Spiel schließen, eigene DLL und Diagnose entfernen\n"
        "und nur gegebenenfalls zuvor gesicherte gleichnamige Dateien zurücklegen.\n"
        "Keine Spielstände sind Teil dieses Pakets.\n"
    )
    manifest = {
        "version": VERSION,
        "kind": "diagnostic-probe",
        "architecture": "x86",
        "sha256": digest,
        "gameplay_hooks": False,
        "script_calls": False,
        "game_loading_verified": False,
    }
    third_party = (
        "Fremde Komponenten im Diagnosepaket\n\n"
        "Die DLL wurde mit Microsoft Visual C++ Release-CRT (x86, Paketfamilie\n"
        "14.44.17.14) statisch gelinkt. Microsoft-Komponenten unterliegen den\n"
        "Microsoft Software License Terms, nicht der MIT-Lizenz des eigenen Codes.\n"
        "Copyright Microsoft Corporation. Alle Rechte vorbehalten.\n"
        "https://learn.microsoft.com/en-us/cpp/windows/redistributing-visual-cpp-files\n\n"
        "SDK-Header, SDK-Library-Archive und Buildwerkzeuge werden nicht mitgeliefert.\n"
    )
    contents = {
        "UnionBatchCraftingProbe.dll": dll,
        "LICENSE": (ROOT / "LICENSE").read_bytes(),
        "FREMDKOMPONENTEN.txt": third_party.encode(),
        "ANLEITUNG.txt": instruction.encode(),
        "manifest.json": (json.dumps(manifest, indent=2) + "\n").encode(),
        "SHA256SUMS": (digest + "  UnionBatchCraftingProbe.dll\n").encode(),
    }
    out.mkdir(exist_ok=True)
    with zipfile.ZipFile(
        archive, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9
    ) as bundle:
        for name, content in sorted(contents.items()):
            item = zipfile.ZipInfo(name, date_time=(1980, 1, 1, 0, 0, 0))
            item.compress_type = zipfile.ZIP_DEFLATED
            item.external_attr = 0o100644 << 16
            bundle.writestr(
                item, content, compress_type=zipfile.ZIP_DEFLATED, compresslevel=9
            )
    print(f"Paket: {archive}")
    print("SHA-256: " + hashlib.sha256(archive.read_bytes()).hexdigest())


if __name__ == "__main__":
    main()
