#!/usr/bin/env python3
"""Run portable regressions and write a summary with --apply.

Build under /tmp/gothic-batch-test.*, run the registered CTest and Python tests,
and retain the logs, JSON and XML. Optionally build Windows x86 artifacts and run
the hosts in a new private Wine prefix. The runner never starts Gothic or Steam
or changes saves or INIs."""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import re
import shutil
import signal
import subprocess
import tempfile
import time
import xml.etree.ElementTree as ET
from cool_local_facts import local_files

ROOT = Path(__file__).resolve().parents[2]


def positive_seconds(value: str) -> float:
    result = float(value)
    if not 0 < result <= 3600:
        raise argparse.ArgumentTypeError(
            "Timeout muss zwischen 0 und 3600 Sekunden liegen"
        )
    return result


def stop_owned_process(proc: subprocess.Popen) -> None:
    # Only the process group we created with start_new_session.
    try:
        os.killpg(proc.pid, signal.SIGTERM)
    except ProcessLookupError:
        return
    try:
        proc.wait(timeout=3)
    except subprocess.TimeoutExpired:
        try:
            os.killpg(proc.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
        proc.wait()


def run_step(name: str, command: list[str], run_dir: Path, timeout: float) -> dict:
    log = run_dir / (name + ".log")
    start = time.monotonic()
    status = "PASS"
    code = 1
    with log.open("w") as stream:
        try:
            proc = subprocess.Popen(
                command,
                cwd=ROOT,
                stdout=stream,
                stderr=subprocess.STDOUT,
                start_new_session=True,
                env={**os.environ, "PYTHONDONTWRITEBYTECODE": "1"},
            )
            try:
                code = proc.wait(timeout=timeout)
                status = "PASS" if code == 0 else "FAIL"
            except subprocess.TimeoutExpired:
                stop_owned_process(proc)
                code, status = 124, "TIMEOUT"
                stream.write("\nTimeout; eigene Testprozessgruppe beendet.\n")
            except BaseException:
                stop_owned_process(proc)
                raise
        except OSError as error:
            stream.write(str(error) + "\n")
            status, code = "ERROR", 127
    return {
        "name": name,
        "status": status,
        "exit_code": code,
        "seconds": round(time.monotonic() - start, 2),
        "log": str(log),
    }


def native_counts(report: Path) -> dict:
    root = ET.parse(report).getroot()
    cases = list(root.iter("testcase"))
    return {
        "tests": len(cases),
        "failed": sum(
            c.find("failure") is not None or c.find("error") is not None for c in cases
        ),
        "skipped": sum(c.find("skipped") is not None for c in cases),
    }


def error_excerpt(log: Path) -> str:
    with log.open("rb") as stream:
        stream.seek(max(0, log.stat().st_size - 12000))
        lines = stream.read().decode(errors="replace").splitlines()
    return "\n".join(line[:300] for line in lines[-16:])


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--apply", action="store_true", help="Run the tests and write temporary reports"
    )
    parser.add_argument(
        "--sanitizers",
        action="store_true",
        help="Also run the existing ASan/UBSan task",
    )
    parser.add_argument(
        "--windows",
        action="store_true",
        help="Also build for Windows x86 and run the hosts in a private Wine prefix",
    )
    parser.add_argument(
        "--local-facts",
        nargs=2,
        type=Path,
        metavar=("GAME_EXE", "LHIVER_ARCHIVE"),
        help="Read original files for positive x86 alchemy tests",
    )
    parser.add_argument(
        "--timeout-seconds",
        type=positive_seconds,
        default=300,
        help="Timeout per step in seconds (default 300)",
    )
    args = parser.parse_args(argv)
    if not args.apply:
        print(
            "PLAN: isoliert bauen -> registrierte CTest-Tests -> Python-Tests"
            + (" -> ASan/UBSan" if args.sanitizers else "")
            + (" -> Windows-x86-Build -> isolierte Winehosts" if args.windows else "")
        )
        print("Ohne --apply keine Prozesse, Dateien oder Spielaenderungen.")
        return 0
    if args.windows and not args.local_facts:
        parser.error("--windows benoetigt --local-facts GAME_EXE LHIVER_ARCHIVE")
    cool_files = (
        [str(path) for path in local_files(*args.local_facts)]
        if args.local_facts
        else []
    )
    if not cool_files:
        print(
            "SKIP local contracts: the original game executable, mod archive and matching modules are required."
        )
    run_dir = Path(
        tempfile.mkdtemp(prefix="gothic-batch-test.report-", dir="/tmp")
    ).resolve()
    if run_dir.parent != Path("/tmp").resolve() or not run_dir.name.startswith(
        "gothic-batch-test.report-"
    ):
        raise RuntimeError("Ungueltiges temporaeres Testverzeichnis")
    build_dir = run_dir / "native"
    steps = []
    interrupted = False
    try:
        for name, command in [
            (
                "configure",
                [
                    "cmake",
                    "-S",
                    str(ROOT),
                    "-B",
                    str(build_dir),
                    "-G",
                    "Ninja",
                    "-DCMAKE_BUILD_TYPE=Release",
                ],
            ),
            ("build", ["cmake", "--build", str(build_dir), "--parallel", "2"]),
        ]:
            steps.append(run_step(name, command, run_dir, args.timeout_seconds))
            if steps[-1]["exit_code"] != 0:
                break
        if steps[-1]["exit_code"] == 0:
            junit = run_dir / "native.xml"
            native = run_step(
                "native",
                [
                    "ctest",
                    "--test-dir",
                    str(build_dir),
                    "--output-on-failure",
                    "--no-tests=error",
                    "--output-junit",
                    str(junit),
                ],
                run_dir,
                args.timeout_seconds,
            )
            try:
                native.update(native_counts(junit))
            except (OSError, ET.ParseError) as error:
                native["report_error"] = str(error)
                native["status"], native["exit_code"] = "ERROR", 1
            steps.append(native)
            if cool_files:
                steps.append(
                    run_step(
                        "cool-local-contracts",
                        [
                            str(build_dir / "ubc_cool_predebit_reader_tests"),
                            "--local-files",
                            *cool_files,
                        ],
                        run_dir,
                        args.timeout_seconds,
                    )
                )
                steps.append(
                    run_step(
                        "bow-local-contracts",
                        [
                            str(build_dir / "ubc_bow_item_runtime_tests"),
                            "--local-files",
                            *cool_files,
                        ],
                        run_dir,
                        args.timeout_seconds,
                    )
                )
                engine, _, zbug, union, *_ = cool_files
                steps.append(
                    run_step(
                        "grind-entry-local-contracts",
                        [
                            str(build_dir / "ubc_zbug_end_compat_tests"),
                            zbug,
                            union,
                            engine,
                        ],
                        run_dir,
                        args.timeout_seconds,
                    )
                )
            if args.local_facts:
                steps.append(
                    run_step(
                        "menu-local-contracts",
                        [
                            str(build_dir / "ubc_native_crafting_ui_tests"),
                            "--local-exe",
                            str(args.local_facts[0]),
                        ],
                        run_dir,
                        args.timeout_seconds,
                    )
                )
        # Evaluate the independent Python tests even if C++ fails.
        helpers = run_step(
            "tools", ["mise", "run", "test:tools"], run_dir, args.timeout_seconds
        )
        matches = re.findall(
            r"Ran (\d+) tests?", Path(helpers["log"]).read_text(errors="replace")
        )
        if matches:
            helpers["tests"] = int(matches[-1])
        elif helpers["exit_code"] == 0:
            helpers["status"], helpers["exit_code"] = "ERROR", 1
            helpers["report_error"] = "Keine Python-Testanzahl gefunden"
        if helpers.get("tests") == 0:
            helpers["status"], helpers["exit_code"] = "ERROR", 1
        steps.append(helpers)
        if args.sanitizers:
            command = ["mise", "run", "test:sanitizers"]
            if cool_files:
                command += ["--", "--cool-local-files", *cool_files]
            steps.append(run_step("sanitizers", command, run_dir, args.timeout_seconds))
        if args.windows:
            steps.append(
                run_step(
                    "windows-build",
                    ["mise", "run", "build:windows"],
                    run_dir,
                    args.timeout_seconds,
                )
            )
            if steps[-1]["exit_code"] == 0:
                steps.append(
                    run_step(
                        "windows-hosts",
                        [
                            "python3",
                            "tools/test/smoke-test-WRITES.py",
                            "--apply",
                            "--crafting",
                            "--local-facts",
                            *(str(p) for p in args.local_facts),
                        ],
                        run_dir,
                        args.timeout_seconds,
                    )
                )
    except KeyboardInterrupt:
        interrupted = True
    finally:
        # Remove only the build we just created. Never remove other builds.
        if build_dir.is_dir() and not build_dir.is_symlink():
            shutil.rmtree(build_dir)
    failed = interrupted or any(step["exit_code"] != 0 for step in steps)
    result = {
        "status": "INTERRUPTED" if interrupted else ("FAIL" if failed else "PASS"),
        "scope": "portable_and_isolated_windows"
        if args.windows
        else "native_and_python_only",
        "world_tested": False,
        "physical_controller_tested": False,
        "steps": steps,
    }
    report = run_dir / "summary.json"
    report.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n")
    print(
        "RESULT: "
        + result["status"]
        + " ("
        + ("portable und isolierte x86-Tests" if args.windows else "portable Tests")
        + "; kein Spielwelt-/Controllerbeleg)"
    )
    for step in steps:
        count = f"; {step['tests']} Tests" if "tests" in step else ""
        print(f"{step['name']}: {step['status']}{count}; {step['seconds']}s")
        if step["exit_code"] != 0:
            print(error_excerpt(Path(step["log"])))
            if "report_error" in step:
                print(step["report_error"])
    print("REPORT: " + str(report))
    return 130 if interrupted else (1 if failed else 0)


if __name__ == "__main__":
    raise SystemExit(main())
