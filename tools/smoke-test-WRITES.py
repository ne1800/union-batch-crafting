#!/usr/bin/env python3
"""Test DLL loading in a private temporary Wine prefix with --apply."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
from cool_local_facts import local_files
from world.cool_check_fields import fields_by_phase

ROOT = Path(__file__).resolve().parents[1]
PREFIX = "gothic-batch-test."


def validate_temporary(path: Path) -> None:
    if path.is_symlink() or path.resolve() != path or path.parent != Path("/tmp"):
        raise RuntimeError("Ungültiger Testpfad.")
    if (
        not path.name.startswith(PREFIX)
        or path.name == PREFIX
        or path.stat().st_uid != os.getuid()
    ):
        raise RuntimeError("Testverzeichnis ist nicht eindeutig task-eigen.")


def windows_path(path: Path) -> str:
    return "Z:" + str(path).replace("/", "\\")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--apply",
        action="store_true",
        help="Create a private test prefix and run the hosts",
    )
    parser.add_argument(
        "--keep", action="store_true", help="Keep temporary test evidence after the run"
    )
    parser.add_argument(
        "--menu",
        action="store_true",
        help="Check menu probe rejection in a foreign process",
    )
    parser.add_argument(
        "--station",
        action="store_true",
        help="Check passive station probe rejection in a foreign process",
    )
    parser.add_argument(
        "--selection",
        action="store_true",
        help="Check original selection probe rejection for both activation modes in a foreign process",
    )
    parser.add_argument(
        "--batch",
        action="store_true",
        help="Check the quantity plugin, CallFunc bridge and both activation modes in a foreign process",
    )
    parser.add_argument(
        "--recipes",
        action="store_true",
        help="Check the recipe menu plugin and both activation modes in a foreign process",
    )
    parser.add_argument(
        "--crafting",
        action="store_true",
        help="Check the shared station plugin and native adapters in a private prefix",
    )
    parser.add_argument(
        "--local-facts",
        nargs=2,
        type=Path,
        metavar=("GAME_EXE", "LHIVER_ARCHIVE"),
        help="Read original files for positive alchemy and material contract checks",
    )
    args = parser.parse_args()
    if not args.apply:
        print(
            "Plan: eigener /tmp/gothic-batch-test.*-Prefix, DLL-Ladetests und Aufräumen; Start nur mit --apply."
        )
        return
    wine = shutil.which("wine")
    server = shutil.which("wineserver")
    if wine is None or server is None:
        raise SystemExit("wine und wineserver werden benötigt.")
    facts = []
    if args.local_facts:
        facts = [windows_path(p.resolve(strict=True)) for p in args.local_facts]
    if args.crafting and not facts:
        raise SystemExit(
            "--crafting benoetigt --local-facts GAME_EXE LHIVER_ARCHIVE fuer den positiven Zusatzalchemie-Host."
        )
    cool_files = (
        [windows_path(p) for p in local_files(*args.local_facts)]
        if args.crafting
        else []
    )
    build = ROOT / "build/windows"
    recipe_tests = (
        "ubc_recipe_catalog_tests.exe",
        "ubc_recipe_material_tests.exe",
        "ubc_recipe_result_tests.exe",
        "ubc_recipe_runtime_tests.exe",
        "ubc_recipe_session_tests.exe",
        "ubc_recipe_menu_tests.exe",
        "ubc_recipe_navigation_tests.exe",
        "ubc_recipe_handoff_tests.exe",
        "ubc_recipe_snapshot_tests.exe",
        "ubc_station_recipe_catalog_tests.exe",
        "ubc_station_recipe_material_tests.exe",
        "ubc_station_recipe_navigation_tests.exe",
        "ubc_station_recipe_result_tests.exe",
        "ubc_station_recipe_runtime_tests.exe",
        "ubc_station_recipe_snapshot_tests.exe",
        "ubc_post_remove_choice_tests.exe",
        "ubc_crafting_catalog_tests.exe",
        "ubc_crafting_session_tests.exe",
        "ubc_crafting_menu_tests.exe",
        "ubc_crafting_handoff_tests.exe",
        "ubc_ammo_presentation_tests.exe",
    )
    extra_tests = tuple(
        "ubc_" + name + "_tests.exe"
        for name in (
            "recipe_family_table",
            "cool_hotblade_debit_bridge",
            "cool_predebit_bridge",
            "crafting_ui_diagnostics",
            "grind_ui_readiness",
            "alchemy_extra_snapshot",
            "alchemy_extra_result",
            "alchemy_extra_navigation",
            "alchemy_extra_execution",
            "alchemy_extra_dependencies",
            "alchemy_extra_dependency_reader",
            "alchemy_extra_writer",
            "alchemy_extra_end_snapshot",
            "crafting_alchemy_extra_reader",
            "vob_locator_compat",
            "vob_inventory_insert_compat",
            "zbug_end_compat",
            "anvil_recipe_catalog",
            "anvil_recipe_runtime",
            "anvil_recipe_navigation",
            "anvil_recipe_snapshot",
            "ore_recipe_catalog",
            "ore_recipe_runtime",
            "ore_recipe_navigation",
            "ore_recipe_snapshot",
            "mixed_hotsteel",
            "alchemy_flask_debit_bridge",
            "windows_module_identity",
            "spirit_recipe_catalog",
            "spirit_recipe_runtime",
            "spirit_recipe_navigation",
            "spirit_recipe_snapshot",
            "spirit_execution",
            "spirit_result",
            "spirit_end_snapshot",
            "spirit_crafting_reader",
            "crafting_spirit_projection",
            "crafting_bow_projection",
            "crafting_bow_execution",
            "crafting_spirit_shared",
            "spirit_writer",
            "tobacco_recipe",
            "bow_recipe_catalog",
            "bow_recipe_runtime",
            "bow_recipe_navigation",
            "bow_recipe_snapshot",
            "bow_item_path",
            "bow_item_code",
            "bow_item_native",
            "bow_item_renderer",
            "bow_item_runtime",
            "grind_recipe_plan",
            "grind_recipe_material",
            "grind_recipe_snapshot",
            "grind_item_binding",
            "grind_item_path",
            "grind_item_runtime",
            "grind_preview_batch",
            "grind_zbug_unpack",
            "grind_destroy_observation",
            "grind_guild_snapshot",
            "grind_recipe_runtime",
            "grind_recipe_navigation",
            "cool_recipe_plan",
            "cool_recipe_runtime",
            "cool_recipe_snapshot",
            "cool_recipe_navigation",
            "cool_recipe_facts",
            "cool_parser_prefix",
            "cool_hotblade_debit",
            "cool_predebit_contract",
            "cool_predebit_reader",
            "cool_nofx_reader",
            "cool_message_malloc_reader",
            "cool_direct_contract",
            "cool_actor_preview",
            "destroy_remove_callsite",
            "windows_destroy_remove_callsite",
            "cool_native_path",
            "cool_destroy_dispatch",
            "cool_destroy_observation",
            "crafting_cool_shared",
            "crafting_grind_shared",
        )
    )
    files = [
        build / "UnionBatchCraftingProbe.dll",
        build / "ubc_probe_host.exe",
        build / "ubc_core_tests.exe",
    ]
    if args.menu:
        files += [
            build / "UnionBatchCraftingMenuProbe.dll",
            build / "ubc_menu_probe_host.exe",
            build / "ubc_sha256_tests.exe",
            build / "ubc_menu_model_tests.exe",
        ]
    if args.station:
        files += [
            build / "UnionBatchCraftingStationProbe.dll",
            build / "ubc_station_probe_host.exe",
            build / "ubc_station_snapshot_tests.exe",
            build / "ubc_runtime_script_tests.exe",
            build / "ubc_hand_snapshot_tests.exe",
            build / "ubc_native_hook_tests.exe",
            build / "ubc_info_snapshot_tests.exe",
            build / "ubc_packed_inventory_tests.exe",
            build / "ubc_selection_session_tests.exe",
            build / "ubc_selection_stock_tests.exe",
            build / "ubc_frame_bridge_tests.exe",
            build / "ubc_selection_snapshot_tests.exe",
            build / "ubc_batch_session_tests.exe",
            build / "ubc_batch_menu_tests.exe",
            build / "ubc_amount_patch_tests.exe",
            build / "ubc_batch_result_tests.exe",
        ]
    if args.selection:
        files += [
            build / "UnionBatchCraftingSelectionProbe.dll",
            build / "ubc_selection_probe_host.exe",
            build / "ubc_selection_effect_guard_tests.exe",
        ]
    if args.batch:
        files += [
            build / "UnionBatchCrafting.dll",
            build / "ubc_batch_probe_host.exe",
            build / "ubc_batch_call_bridge_tests.exe",
        ]
    if args.recipes:
        files += [
            build / "UnionBatchCraftingRecipes.dll",
            build / "ubc_recipe_probe_host.exe",
            build / "ubc_recipe_effect_guard_tests.exe",
            build / "ubc_ammo_presentation_bridge_tests.exe",
        ]
    if args.station or args.batch or args.crafting:
        files += [build / name for name in recipe_tests]
    if args.crafting:
        files.append(build / "ubc_player_engine_tests.exe")
        files += [
            build / name
            for name in (
                "UnionBatchCraftingExtended.dll",
                "ubc_crafting_probe_host.exe",
                "ubc_crafting_result_tests.exe",
                "ubc_native_crafting_ui_tests.exe",
                "ubc_ammo_runtime_tests.exe",
                "ubc_crafting_effect_guard_tests.exe",
                "ubc_alchemy_flask_debit_tests.exe",
                "ubc_station_end_contract_tests.exe",
                "ubc_native_crafting_ammo_tests.exe",
                "ubc_alchemy_recipe_catalog_tests.exe",
                "ubc_alchemy_flask_runtime_tests.exe",
            )
        ]
    if args.crafting:
        files += [build / name for name in extra_tests]
        files.append(build / "ubc_cool_predebit_test_host.dll")
    if not all(p.is_file() for p in files):
        raise SystemExit("Windows-Build ist unvollständig.")
    work = Path(tempfile.mkdtemp(prefix=PREFIX, dir="/tmp"))
    validate_temporary(work)
    environment = os.environ.copy()
    for key in list(environment):
        if key.upper().startswith("STEAM") or key in ("LD_PRELOAD", "LD_LIBRARY_PATH"):
            environment.pop(key)
    environment.update(
        {
            "WINEPREFIX": str(work / "prefix"),
            "WINEARCH": "win64",
            "WINEDEBUG": "-all",
            "WINEDLLOVERRIDES": "winemenubuilder.exe=d;mscoree,mshtml=d",
            "DISPLAY": "",
            "WAYLAND_DISPLAY": "",
            "TMPDIR": str(work),
        }
    )
    success = False
    try:
        payload = work / "Probe Ä mit Leerzeichen"
        payload.mkdir()
        for source in files:
            shutil.copy2(source, payload / source.name)
        dll = payload / files[0].name
        host = payload / files[1].name
        core = payload / files[2].name
        diagnostic = payload / "UnionBatchCraftingProbe.log"
        with (work / "wine.log").open("w") as log:

            def run(arguments: list[str], timeout: int = 120) -> str:
                result = subprocess.run(
                    [wine, *arguments],
                    env=environment,
                    cwd=work,
                    stdout=subprocess.PIPE,
                    stderr=log,
                    text=True,
                    timeout=timeout,
                )
                with (work / "hosts.log").open("a") as host_log:
                    host_log.write(
                        Path(arguments[0]).name
                        + " exit="
                        + str(result.returncode)
                        + "\n"
                        + result.stdout
                        + "\n"
                    )
                if result.returncode:
                    raise RuntimeError(
                        f"{Path(arguments[0]).name} fehlgeschlagen ({result.returncode}); Details: {work / 'hosts.log'}"
                    )
                return result.stdout.strip()

            print(run([str(core)]), flush=True)
            if args.station or args.batch or args.crafting:
                for name in recipe_tests:
                    print(run([str(payload / name)]), flush=True)
            print(run([str(host), windows_path(dll)]), flush=True)
            if diagnostic.exists():
                raise RuntimeError(
                    "Fremdprozess hat unerwartet eine Ziel-Logdatei erzeugt."
                )
            synthetic = payload / "synthetic_header_host.exe"
            data = bytearray(host.read_bytes())
            pe = struct.unpack_from("<I", data, 0x3C)[0]
            if data[:2] != b"MZ" or data[pe : pe + 4] != b"PE\0\0":
                raise RuntimeError("Eigener Testhost ist kein PE-Programm.")
            if struct.unpack_from("<I", data, pe + 24 + 28)[0] != 0x00400000:
                raise RuntimeError("Testhost hat eine unerwartete Basisadresse.")
            struct.pack_into("<I", data, pe + 8, 0x3F966D87)
            struct.pack_into("<I", data, pe + 24 + 56, 0x00752000)
            # The positive header contract requires ImageBase 0x00400000. With
            # ASLR, Wine writes the loaded base back into the PE header. Load only
            # this temporary fixture at a fixed address. Do not change any DLL.
            characteristics = struct.unpack_from("<H", data, pe + 24 + 70)[0]
            struct.pack_into("<H", data, pe + 24 + 70, characteristics & ~0x0040)
            synthetic.write_bytes(data)
            print(run([str(synthetic), windows_path(dll), "1"]), flush=True)
            expected = "probe_version=1\ntarget_header_match=true\ngameplay_hooks=false\nscript_calls=false\n"
            if not diagnostic.is_file() or diagnostic.read_text() != expected:
                raise RuntimeError(
                    "Synthetischer Zieltest hat keine korrekte Diagnose erzeugt."
                )
            if args.menu:
                print(run([str(payload / "ubc_sha256_tests.exe")]), flush=True)
                print(run([str(payload / "ubc_menu_model_tests.exe")]), flush=True)
                menu_dll = payload / "UnionBatchCraftingMenuProbe.dll"
                menu_host = payload / "ubc_menu_probe_host.exe"
                marker = payload / "UnionBatchCraftingMenuProbe.enable-once"
                marker.write_bytes(b"UI_PROBE_ONLY_V1\n")
                print(run([str(menu_host), windows_path(menu_dll)]), flush=True)
                spoofed = payload / "synthetic_menu_host.exe"
                spoofed_data = bytearray(menu_host.read_bytes())
                menu_pe = struct.unpack_from("<I", spoofed_data, 0x3C)[0]
                struct.pack_into("<I", spoofed_data, menu_pe + 8, 0x3F966D87)
                struct.pack_into("<I", spoofed_data, menu_pe + 24 + 56, 0x00752000)
                spoofed.write_bytes(spoofed_data)
                print(run([str(spoofed), windows_path(menu_dll)]), flush=True)
                if (
                    payload / "UnionBatchCraftingMenuProbe.log"
                ).exists() or marker.read_bytes() != b"UI_PROBE_ONLY_V1\n":
                    raise RuntimeError(
                        "Fremder Menü-Testhost hat Diagnose oder Aktivierung verändert."
                    )
            if args.station:
                print(
                    run([str(payload / "ubc_station_snapshot_tests.exe")]), flush=True
                )
                print(run([str(payload / "ubc_runtime_script_tests.exe")]), flush=True)
                print(run([str(payload / "ubc_hand_snapshot_tests.exe")]), flush=True)
                print(run([str(payload / "ubc_native_hook_tests.exe")]), flush=True)
                print(run([str(payload / "ubc_info_snapshot_tests.exe")]), flush=True)
                print(
                    run([str(payload / "ubc_packed_inventory_tests.exe")]), flush=True
                )
                print(
                    run([str(payload / "ubc_selection_session_tests.exe")]), flush=True
                )
                print(run([str(payload / "ubc_selection_stock_tests.exe")]), flush=True)
                print(run([str(payload / "ubc_frame_bridge_tests.exe")]), flush=True)
                print(
                    run([str(payload / "ubc_selection_snapshot_tests.exe")]), flush=True
                )
                print(run([str(payload / "ubc_batch_session_tests.exe")]), flush=True)
                print(run([str(payload / "ubc_batch_menu_tests.exe")]), flush=True)
                print(run([str(payload / "ubc_amount_patch_tests.exe")]), flush=True)
                print(run([str(payload / "ubc_batch_result_tests.exe")]), flush=True)
                station_dll = payload / "UnionBatchCraftingStationProbe.dll"
                station_host = payload / "ubc_station_probe_host.exe"
                station_marker = payload / "UnionBatchCraftingStationProbe.enable-once"
                station_marker.write_bytes(b"STATION_OBSERVER_V1\n")
                print(run([str(station_host), windows_path(station_dll)]), flush=True)
                spoofed = payload / "synthetic_station_host.exe"
                spoofed_data = bytearray(station_host.read_bytes())
                station_pe = struct.unpack_from("<I", spoofed_data, 0x3C)[0]
                struct.pack_into("<I", spoofed_data, station_pe + 8, 0x3F966D87)
                struct.pack_into("<I", spoofed_data, station_pe + 24 + 56, 0x00752000)
                spoofed.write_bytes(spoofed_data)
                print(run([str(spoofed), windows_path(station_dll)]), flush=True)
                if (
                    payload / "UnionBatchCraftingStationProbe.log"
                ).exists() or station_marker.read_bytes() != b"STATION_OBSERVER_V1\n":
                    raise RuntimeError(
                        "Fremder Stations-Testhost hat Diagnose oder Aktivierung verändert."
                    )
            if args.selection:
                print(
                    run([str(payload / "ubc_selection_effect_guard_tests.exe")]),
                    flush=True,
                )
                selected_dll = payload / "UnionBatchCraftingSelectionProbe.dll"
                selected_host = payload / "ubc_selection_probe_host.exe"
                marker = payload / "UnionBatchCraftingSelectionProbe.enable-once"
                spoofed = payload / "synthetic_selection_host.exe"
                spoofed_data = bytearray(selected_host.read_bytes())
                selected_pe = struct.unpack_from("<I", spoofed_data, 0x3C)[0]
                struct.pack_into("<I", spoofed_data, selected_pe + 8, 0x3F966D87)
                struct.pack_into("<I", spoofed_data, selected_pe + 24 + 56, 0x00752000)
                spoofed.write_bytes(spoofed_data)
                for activation in (b"SELECTION_UI_V1\n", b"ORIGINAL_SELECTION_V1\n"):
                    marker.write_bytes(activation)
                    for executable in (selected_host, spoofed):
                        print(
                            run([str(executable), windows_path(selected_dll)]),
                            flush=True,
                        )
                        if (
                            payload / "UnionBatchCraftingSelectionProbe.log"
                        ).exists() or marker.read_bytes() != activation:
                            raise RuntimeError(
                                "Fremder Originalauswahl-Host hat Marker/Log verändert."
                            )
            if args.batch:
                print(
                    run([str(payload / "ubc_batch_call_bridge_tests.exe")]), flush=True
                )
                batch_dll = payload / "UnionBatchCrafting.dll"
                batch_host = payload / "ubc_batch_probe_host.exe"
                marker = payload / "UnionBatchCrafting.enable-once"
                spoofed = payload / "synthetic_batch_host.exe"
                spoofed_data = bytearray(batch_host.read_bytes())
                batch_pe = struct.unpack_from("<I", spoofed_data, 0x3C)[0]
                struct.pack_into("<I", spoofed_data, batch_pe + 8, 0x3F966D87)
                struct.pack_into("<I", spoofed_data, batch_pe + 24 + 56, 0x00752000)
                spoofed.write_bytes(spoofed_data)
                for activation in (b"BATCH_UI_V1\n", b"FREE_AMOUNTS_V1\n"):
                    marker.write_bytes(activation)
                    for executable in (batch_host, spoofed):
                        print(
                            run([str(executable), windows_path(batch_dll)]), flush=True
                        )
                        if (
                            payload / "UnionBatchCrafting.log"
                        ).exists() or marker.read_bytes() != activation:
                            raise RuntimeError(
                                "Fremder Mengen-Host hat Marker/Log verändert."
                            )
            if args.recipes:
                for mode in (
                    "normal",
                    "receipt",
                    "fault-0",
                    "fault-1",
                    "fault-2",
                    "foreign",
                    "reentry",
                    "decision",
                    "disable",
                ):
                    print(
                        run(
                            [
                                str(payload / "ubc_ammo_presentation_bridge_tests.exe"),
                                mode,
                            ]
                        ),
                        flush=True,
                    )
                print(
                    run([str(payload / "ubc_recipe_effect_guard_tests.exe")]),
                    flush=True,
                )
                recipe_dll = payload / "UnionBatchCraftingRecipes.dll"
                recipe_host = payload / "ubc_recipe_probe_host.exe"
                marker = payload / "UnionBatchCraftingRecipes.enable-once"
                spoofed = payload / "synthetic_recipe_host.exe"
                spoofed_data = bytearray(recipe_host.read_bytes())
                recipe_pe = struct.unpack_from("<I", spoofed_data, 0x3C)[0]
                struct.pack_into("<I", spoofed_data, recipe_pe + 8, 0x3F966D87)
                struct.pack_into("<I", spoofed_data, recipe_pe + 24 + 56, 0x00752000)
                spoofed.write_bytes(spoofed_data)
                for activation in (b"RECIPE_UI_V1\n", b"RECIPE_AMOUNTS_V1\n"):
                    marker.write_bytes(activation)
                    for executable in (recipe_host, spoofed):
                        print(
                            run([str(executable), windows_path(recipe_dll)]), flush=True
                        )
                        if (
                            payload / "UnionBatchCraftingRecipes.log"
                        ).exists() or marker.read_bytes() != activation:
                            raise RuntimeError(
                                "Fremder Rezept-Host hat Marker/Log verändert."
                            )
            if args.crafting:
                # The host directory contains a space and a non-ASCII character. Its working directory differs.
                game_ini = payload / "Gothic.INI"
                for option, game, expected in (
                    (None, "de", "de"),
                    ("auto", "en", "en"),
                    ("en", "de", "en"),
                    ("de", "en", "de"),
                    ("other", "de", "en"),
                ):
                    if option is not None:
                        game_ini.write_bytes(
                            f"[UNIONBATCHCRAFTING]\r\nLanguage={option}\r\n".encode(
                                "ascii"
                            )
                        )
                    print(
                        run(
                            [
                                str(payload / "ubc_native_crafting_ui_tests.exe"),
                                "--check-game-ini",
                                game,
                                expected,
                            ]
                        ),
                        flush=True,
                    )
                game_ini.unlink()
                for name in (
                    "ubc_crafting_result_tests.exe",
                    "ubc_native_crafting_ui_tests.exe",
                    "ubc_ammo_runtime_tests.exe",
                    "ubc_alchemy_flask_debit_tests.exe",
                    "ubc_station_end_contract_tests.exe",
                    "ubc_alchemy_recipe_catalog_tests.exe",
                    "ubc_alchemy_flask_runtime_tests.exe",
                ):
                    print(run([str(payload / name)]), flush=True)
                print(
                    run([str(payload / "ubc_crafting_effect_guard_tests.exe")]),
                    flush=True,
                )
                for mode in (
                    "positive",
                    "admission",
                    *(f"negative-{i}" for i in range(29)),
                    *(f"install-{i}" for i in range(5)),
                ):
                    print(
                        run(
                            [str(payload / "ubc_native_crafting_ammo_tests.exe"), mode]
                        ),
                        flush=True,
                    )
                for name in extra_tests:
                    arguments = [str(payload / name)]
                    if name in (
                        "ubc_cool_predebit_bridge_tests.exe",
                        "ubc_cool_predebit_reader_tests.exe",
                    ):
                        arguments += ["--local-files", *cool_files]
                    elif name in (
                        "ubc_bow_recipe_navigation_tests.exe",
                        "ubc_tobacco_recipe_tests.exe",
                    ):
                        arguments += ["--local-archive", facts[1]]
                    elif name in (
                        "ubc_alchemy_extra_writer_tests.exe",
                        "ubc_alchemy_extra_execution_tests.exe",
                        "ubc_crafting_alchemy_extra_reader_tests.exe",
                        "ubc_spirit_execution_tests.exe",
                        "ubc_spirit_crafting_reader_tests.exe",
                        "ubc_spirit_writer_tests.exe",
                        "ubc_grind_recipe_plan_tests.exe",
                        "ubc_cool_recipe_facts_tests.exe",
                    ):
                        arguments += ["--local-facts", *facts]
                    elif name == "ubc_mixed_hotsteel_tests.exe":
                        arguments += facts[:1]
                    elif name == "ubc_recipe_family_table_tests.exe":
                        arguments.append(
                            windows_path(ROOT / "data/rezeptfamilien-v1.tsv")
                        )
                    output = run(arguments)
                    if name == "ubc_cool_predebit_bridge_tests.exe":
                        expected = fields_by_phase()
                        observed = {p: set() for p in expected}
                        reports = 0
                        for line in output.splitlines():
                            if not line.startswith('{"event":"cool_check'):
                                continue
                            row = json.loads(line)
                            if row["event"] == "cool_check_value":
                                observed[row["phase"]].add(row["field"])
                            elif row["event"] == "cool_checks_complete":
                                if observed != expected:
                                    raise RuntimeError(
                                        "Unvollständiger Prüffeldkatalog im isolierten Host"
                                    )
                                observed = {p: set() for p in expected}
                                reports += 1
                        if reports != 2:
                            raise RuntimeError(
                                "Positive und negative Durchlaufdiagnose im Host erforderlich"
                            )
                        output = output.splitlines()[-1]
                    print(output, flush=True)
                crafting_dll = payload / "UnionBatchCraftingExtended.dll"
                crafting_host = payload / "ubc_crafting_probe_host.exe"
                marker = payload / "UnionBatchCraftingExtended.enable-once"
                spoofed = payload / "synthetic_crafting_host.exe"
                spoofed_data = bytearray(crafting_host.read_bytes())
                crafting_pe = struct.unpack_from("<I", spoofed_data, 0x3C)[0]
                struct.pack_into("<I", spoofed_data, crafting_pe + 8, 0x3F966D87)
                struct.pack_into("<I", spoofed_data, crafting_pe + 24 + 56, 0x00752000)
                spoofed.write_bytes(spoofed_data)
                for activation in (
                    b"EXTENDED_UI_V1\n",
                    b"EXTENDED_AMOUNTS_V1\n",
                    b"EXTENDED_GRIND_MEASURE_V1\n",
                ):
                    marker.write_bytes(activation)
                    for executable in (crafting_host, spoofed):
                        print(
                            run([str(executable), windows_path(crafting_dll)]),
                            flush=True,
                        )
                        if (
                            payload / "UnionBatchCraftingExtended.log"
                        ).exists() or marker.read_bytes() != activation:
                            raise RuntimeError(
                                "Fremder Stations-Host hat Marker/Log verändert."
                            )
                print(
                    run([str(payload / "ubc_player_engine_tests.exe"), facts[0]]),
                    flush=True,
                )
                # Pin the synthetic host at the target base to reach the real
                # player activation branch. Its engine-site contracts must fail.
                flags = struct.unpack_from("<H", spoofed_data, crafting_pe + 24 + 70)[0]
                struct.pack_into(
                    "<H", spoofed_data, crafting_pe + 24 + 70, flags & ~0x40
                )
                spoofed.write_bytes(spoofed_data)
                player = payload / "player"
                player.mkdir()
                player_dll = player / crafting_dll.name
                shutil.copy2(build / "player" / crafting_dll.name, player_dll)
                player_marker = player / marker.name
                player_log = player / "UnionBatchCraftingExtended.log"
                activation = b"EXTENDED_AMOUNTS_V1\n"
                marker_name = marker.name.encode("utf-16le")
                if (
                    marker_name not in crafting_dll.read_bytes()
                    or marker_name in player_dll.read_bytes()
                ):
                    raise RuntimeError("Unexpected activation variant in compiled DLL")
                marker.write_bytes(activation)
                activation_host = payload / "ubc_crafting_effect_guard_tests.exe"
                print(run([str(activation_host), "--marker-present"]), flush=True)
                if marker.exists():
                    raise RuntimeError("Test activation failed to consume its marker")
                print(run([str(activation_host), "--marker-absent"]), flush=True)
                ini = payload / "Gothic.INI"
                ini.unlink(missing_ok=True)
                print(run([str(spoofed), windows_path(player_dll)]), flush=True)
                if "engine_station_disabled" not in player_log.read_text():
                    raise RuntimeError(
                        "Default player activation or engine guard failed"
                    )
                player_marker.write_bytes(activation)
                player_log.write_text("old")
                print(run([str(spoofed), windows_path(player_dll)]), flush=True)
                if (
                    "engine_station_disabled" not in player_log.read_text()
                    or player_marker.read_bytes() != activation
                ):
                    raise RuntimeError(
                        "Repeated player activation or marker isolation failed"
                    )
                player_log.unlink()
                ini.write_text("[UNIONBATCHCRAFTING]\nEnabled=0\n")
                print(run([str(spoofed), windows_path(player_dll)]), flush=True)
                if player_log.exists() or player_marker.read_bytes() != activation:
                    raise RuntimeError("Enabled=0 failed")
        print(
            "Negative und synthetische positive DLL-Ladetests bestanden; kein Gothic-Prozess gestartet."
        )
        for checked_dll in sorted(payload.glob("*.dll")):
            print(
                checked_dll.name
                + " SHA-256: "
                + hashlib.sha256(checked_dll.read_bytes()).hexdigest()
            )
        success = True
    finally:
        validate_temporary(work)
        # WINEPREFIX is fixed to the new test prefix created above. It is not a user argument.
        try:
            stopped = subprocess.run(
                [server, "-k"],
                env=environment,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
                timeout=30,
                check=False,
            )
            waited = subprocess.run(
                [server, "-w"],
                env=environment,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
                timeout=30,
                check=False,
            )
        except subprocess.TimeoutExpired as error:
            raise RuntimeError(
                f"Wine-Aufräumen nicht abgeschlossen; Testprefix bleibt erhalten: {work}"
            ) from error
        if stopped.returncode != 0 or waited.returncode != 0:
            raise RuntimeError(
                f"Wine-Aufräumen fehlgeschlagen; Testprefix bleibt erhalten: {work}"
            )
        if success and not args.keep:
            validate_temporary(work)
            shutil.rmtree(work)
            print("Eigener Testprefix und temporäre Testdateien entfernt.")
        else:
            print(f"Testnachweise: {work}")


if __name__ == "__main__":
    main()
