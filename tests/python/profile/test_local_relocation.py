"""Local check of the generated script profiles against real scripts.

Skipped unless UBC_LOCAL_MARCELLO_DAT or UBC_LOCAL_DNDR_DAT names a local
reference GOTHIC.DAT; UBC_LOCAL_<CAMPAIGN>_SHIFTED_DAT may add a copy with
moved symbols and code. The scripts are only read. Parser images, binaries
and regenerated headers live in a temporary directory that is removed.
"""

from pathlib import Path
import json
import os
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
TOOLS = ROOT / "tools/profile"
sys.path.insert(0, str(TOOLS))
import catalog_literals  # noqa: E402
import parser_image  # noqa: E402
import script_profile  # noqa: E402
from datfile import OP_PUSH_INT, DatFile  # noqa: E402

HEADERS = {
    "marcello": "marcello_204_fingerprints.hpp",
    "dndr": "dndr_26_fingerprints.hpp",
}
CXX = os.environ.get("CXX", "c++")


def compile_tool(source: str, output: Path) -> Path:
    subprocess.run(
        [
            CXX,
            "-std=c++17",
            "-O2",
            "-I",
            str(ROOT / "include"),
            str(TOOLS / source),
            "-o",
            str(output),
        ],
        check=True,
    )
    return output


class Campaign:
    def __init__(self, name: str, path: str, work: Path, recorder: Path, probe: Path):
        self.name, self.path, self.work, self.probe = name, path, work, probe
        self.dat = DatFile.load(path)
        names = (
            parser_image.runtime_anonymous_names(self.dat, ROOT / "include")
            if name == "marcello"
            else {}
        )
        image = work / f"{name}.img"
        parser_image.write(image, parser_image.build(self.dat, names))
        output = subprocess.run(
            [str(recorder), name, str(image)],
            check=True,
            capture_output=True,
            text=True,
        ).stdout
        recipes = json.loads(output)["recipes"]
        literals = catalog_literals.scan(ROOT / "include")
        self.profile = script_profile.Builder(name, self.dat, recipes, literals).build()

    def relocate(self, dat: DatFile, label: str) -> dict:
        image = self.work / f"{self.name}-{label}.img"
        parser_image.write(image, parser_image.build(dat))
        output = subprocess.run(
            [str(self.probe), self.name, str(image), "5"],
            capture_output=True,
            text=True,
        ).stdout
        image.unlink()
        return json.loads(output)

    def recipe_ids(self, groups) -> set[int]:
        return {group["recipe"] for group in groups}

    def amount_mutation(self) -> tuple[DatFile, set[int]]:
        """Change one literal PUSH_INT in a function only one recipe uses."""
        profile = self.profile
        users: dict[int, set[int]] = {}
        for group in profile.groups:
            for record in group["functions"]:
                users.setdefault(record, set()).add(group["recipe"])
        shared = {r for station in profile.stations for r in station["functions"]}
        for group in profile.groups:
            for record_id in group["functions"]:
                record = profile.functions[record_id]
                if users[record_id] != {group["recipe"]} or record_id in shared:
                    continue
                start = record["start"]
                symbolic = {start + offset for offset in record["symbolic"]}
                for instruction in self.dat.instructions(
                    start, start + record["length"]
                ):
                    if (
                        instruction.opcode == OP_PUSH_INT
                        and instruction.offset not in symbolic
                        and 0 < instruction.operand < 1000
                    ):
                        code = bytearray(self.dat.code)
                        value = instruction.operand + 1
                        code[instruction.offset + 1 : instruction.offset + 5] = (
                            value.to_bytes(4, "little")
                        )
                        mutated = DatFile.parse(bytes(self._with_code(bytes(code))))
                        return mutated, {group["recipe"]}
        raise AssertionError("no recipe-owned literal found")

    def constant_mutation(self) -> tuple[DatFile, set[int]]:
        """Change one constant that only one station share binds."""
        profile = self.profile
        elsewhere: dict[int, set[int]] = {}
        for k, station in enumerate(profile.stations):
            for record in station["data"]:
                elsewhere.setdefault(record, set()).add(k)
        for group in profile.groups:
            for record in group["data"]:
                elsewhere.setdefault(record, set()).add(-1)
        for k, station in enumerate(profile.stations):
            members = [g for g in profile.groups if g["station"] == k]
            if len(members) < 2:
                continue
            for record_id in station["data"]:
                record = profile.data[record_id]
                if record["check_value"] and elsewhere[record_id] == {k}:
                    mutated = DatFile.parse(self.dat_bytes())
                    mutated.symbols[record["index"]].content = [record["value"] + 1]
                    return mutated, self.recipe_ids(members)
        raise AssertionError("no station constant found")

    def dat_bytes(self) -> bytes:
        return Path(self.path).read_bytes()

    def _with_code(self, code: bytes) -> bytes:
        data = self.dat_bytes()
        return data[: len(data) - len(code)] + code


@unittest.skipUnless(
    os.environ.get("UBC_LOCAL_MARCELLO_DAT") or os.environ.get("UBC_LOCAL_DNDR_DAT"),
    "set UBC_LOCAL_MARCELLO_DAT or UBC_LOCAL_DNDR_DAT to a local reference script",
)
class LocalRelocationTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.directory = tempfile.TemporaryDirectory(prefix="ubc-profile-check-")
        cls.work = Path(cls.directory.name)
        cls.recorder = compile_tool("record_catalog_reads.cpp", cls.work / "recorder")
        cls.probe = compile_tool("relocation_probe.cpp", cls.work / "probe")

    @classmethod
    def tearDownClass(cls):
        cls.directory.cleanup()

    def campaigns(self):
        for name in HEADERS:
            path = os.environ.get(f"UBC_LOCAL_{name.upper()}_DAT")
            if path:
                with self.subTest(campaign=name):
                    yield Campaign(name, path, self.work, self.recorder, self.probe)

    def test_relocation(self):
        for campaign in self.campaigns():
            total = len(campaign.profile.groups)
            original = campaign.relocate(campaign.dat, "original")
            print(
                f"{campaign.name} original: {original['active']}/{original['total']} "
                f"identity={original['identity']} {original['ms']} ms",
                file=sys.stderr,
            )
            self.assertTrue(original["identity"])
            self.assertEqual(original["active"], total)
            self.assertLess(original["ms"], 250)
            shifted_path = os.environ.get(
                f"UBC_LOCAL_{campaign.name.upper()}_SHIFTED_DAT"
            )
            if shifted_path:
                shifted = campaign.relocate(DatFile.load(shifted_path), "shifted")
                print(
                    f"{campaign.name} shifted: {shifted['active']}/{shifted['total']} "
                    f"identity={shifted['identity']} {shifted['ms']} ms",
                    file=sys.stderr,
                )
                self.assertFalse(shifted["identity"])
                self.assertEqual(shifted["active"], total)
            for label, (dat, expected) in (
                ("amount", campaign.amount_mutation()),
                ("constant", campaign.constant_mutation()),
            ):
                result = campaign.relocate(dat, label)
                rejected = {entry["recipe"] for entry in result["rejected"]}
                print(
                    f"{campaign.name} {label}: {result['active']}/{result['total']} rejected "
                    f"{sorted(hex(r) for r in rejected)} via "
                    f"{sorted({e['record'] for e in result['rejected']})}",
                    file=sys.stderr,
                )
                self.assertEqual(rejected, expected)
                self.assertEqual(
                    all(e["station"] for e in result["rejected"]), label == "constant"
                )

    def test_generator_reproduces_header(self):
        for name in HEADERS:
            path = os.environ.get(f"UBC_LOCAL_{name.upper()}_DAT")
            if not path:
                continue
            with self.subTest(campaign=name):
                output = self.work / f"generated-{name}"
                for _ in range(2):
                    subprocess.run(
                        [
                            sys.executable,
                            "-B",
                            str(TOOLS / "generate-script-profile-WRITES.py"),
                            "--campaign",
                            name,
                            "--dat",
                            path,
                            "--recorder",
                            str(self.recorder),
                            "--output-dir",
                            str(output),
                            "--apply",
                        ],
                        check=True,
                        capture_output=True,
                    )
                committed = ROOT / "include/ubc/catalogs/generated" / HEADERS[name]
                self.assertEqual(
                    (output / HEADERS[name]).read_bytes(), committed.read_bytes()
                )


if __name__ == "__main__":
    unittest.main()
