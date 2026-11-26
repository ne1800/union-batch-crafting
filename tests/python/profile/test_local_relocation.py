"""Local check of the generated script profiles against real scripts.

Skipped unless UBC_LOCAL_MARCELLO_DAT or UBC_LOCAL_DNDR_DAT names a local
reference GOTHIC.DAT; UBC_LOCAL_<CAMPAIGN>_SHIFTED_DAT may add a copy with
moved symbols and code. The scripts are only read. Parser images, binaries
and regenerated headers live in a temporary directory that is removed.

Coverage: everything a plugin verifier reads for a recipe (symbol slots,
code bytes, string texts) lies in the recipe group or its station share.
The only exemption are block verifiers (tobacco, DNdR alchemy and anvil):
they also hash the owner and choice functions of the other recipes of the
same route and station, which those recipes' groups check. Parity: when an
exact verifier rejects a recipe on a mutated script, the relocation rejects
it too, unless every mutated symbol is such an exemption of that recipe.
"""

from collections import defaultdict
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

BLOCK_ROUTES = ("tobacco", "dndr")
BLOCK_ROLES = ("owner", "choice")

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
        self.recorder = recorder
        self.dat = DatFile.load(path)
        self.recipes = self.record(self.dat, "reference")
        literals = catalog_literals.scan(ROOT / "include")
        self.profile = script_profile.Builder(
            name, self.dat, self.recipes, literals
        ).build()

    def record(self, dat: DatFile, label: str) -> list[dict]:
        names = (
            parser_image.runtime_anonymous_names(dat, ROOT / "include")
            if self.name == "marcello"
            else {}
        )
        image = self.work / f"{self.name}-{label}-reads.img"
        return parser_image.record(self.recorder, self.name, dat, image, names)

    def exact_rejects(self, dat: DatFile, label: str) -> set[int]:
        """Recipes the plugin's exact verifiers reject on dat."""
        return {r["id"] for r in self.record(dat, label) if not r["matches"]}

    def exemptions(self) -> dict[int, set[int]]:
        owned: dict[tuple, set[int]] = defaultdict(set)
        for recipe in self.recipes:
            if script_profile.ROUTES[recipe["route"]] in BLOCK_ROUTES:
                owned[recipe["route"], recipe["station"]] |= {
                    index for role, index in recipe["own"] if role in BLOCK_ROLES
                }
        return {
            recipe["id"]: owned.get((recipe["route"], recipe["station"]), set())
            - {index for _, index in recipe["own"]}
            for recipe in self.recipes
        }

    def coverage_gaps(self) -> list[str]:
        """Verifier reads outside group and station share, per recipe."""
        dat, profile = self.dat, self.profile
        function_index = [record["index"] for record in profile.functions]
        data_index = [record["index"] for record in profile.data]
        texts = {
            text["reference_index"]: profile.functions[text["function"]]
            for text in profile.anonymous_texts
        }
        exemptions = self.exemptions()
        gaps = []
        for group, recipe in zip(profile.groups, self.recipes, strict=True):
            if group["recipe"] != recipe["id"]:
                return [f"group order differs at {recipe['id']:#x}"]
            share = profile.stations[group["station"]]
            functions = {
                function_index[k] for k in group["functions"] + share["functions"]
            }
            records = [profile.data[k] for k in group["data"] + share["data"]]
            data = {data_index[k] for k in group["data"] + share["data"]}
            checked = {record["index"] for record in records if record["text"]}
            exempt = exemptions[recipe["id"]]

            def bound_text(index: int) -> bool:
                carrier = texts.get(index)
                return (
                    carrier is not None
                    and carrier["index"] in functions
                    and not carrier["ignore_text"]
                )

            covered = script_profile.merged(
                dat.extent(dat.symbols[index]) for index in functions | exempt
            )
            label = f"{recipe['id']:#x}"
            for begin, end in recipe["code"]:
                if not any(a <= begin and end <= b for a, b in covered):
                    gaps.append(f"{label}: code {begin}..{end}")
            for slot in recipe["slots"]:
                symbol = dat.symbols[slot]
                if slot in functions | data | exempt:
                    continue
                if symbol.anonymous and bound_text(slot):
                    continue
                gaps.append(
                    f"{label}: slot {symbol.name if not symbol.anonymous else slot}"
                )
            for index in recipe["strings"]:
                symbol = dat.symbols[index]
                if bound_text(index) if symbol.anonymous else index in checked:
                    continue
                gaps.append(
                    f"{label}: text of {symbol.name if not symbol.anonymous else index}"
                )
        return gaps

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

    def amount_mutation(self) -> tuple[DatFile, set[int], set[int]]:
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
                        return mutated, {group["recipe"]}, {record["index"]}
        raise AssertionError("no recipe-owned literal found")

    def constant_mutation(self) -> tuple[DatFile, set[int], set[int]]:
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
                    return mutated, self.recipe_ids(members), {record["index"]}
        raise AssertionError("no station constant found")

    def call_amount_mutation(
        self, function: str, external: str, position: int
    ) -> tuple[DatFile, set[int]] | None:
        """Change the literal argument of an external call in one function."""
        if function not in self.dat.by_name:
            return None
        symbol = self.dat[function]
        start, end = self.dat.extent(symbol)
        code, slot_of = script_profile.consumers(self.dat, start, end)
        for k, instruction in enumerate(code):
            if (
                instruction.opcode == OP_PUSH_INT
                and slot_of.get(k) == ("external", external, position)
                and 0 < instruction.operand < 1000
            ):
                patched = bytearray(self.dat.code)
                patched[instruction.offset + 1 : instruction.offset + 5] = (
                    instruction.operand + 1
                ).to_bytes(4, "little")
                return DatFile.parse(self._with_code(bytes(patched))), {symbol.index}
        raise AssertionError(f"{function}: no literal {external} argument")

    def text_mutation(self, constant: str) -> tuple[DatFile, set[int]] | None:
        """Change the last byte of a named string constant."""
        if constant not in self.dat.by_name:
            return None
        mutated = DatFile.parse(self.dat_bytes())
        symbol = mutated[constant]
        text = symbol.content[0]
        symbol.content = [text[:-1] + (b"Y" if text.endswith(b"X") else b"X")]
        return mutated, {symbol.index}

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
            for label, (dat, expected, mutated) in (
                ("amount", campaign.amount_mutation()),
                ("constant", campaign.constant_mutation()),
            ):
                result, rejected, _ = self.parity(campaign, dat, label, mutated)
                self.assertEqual(rejected, expected)
                self.assertEqual(
                    all(e["station"] for e in result["rejected"]), label == "constant"
                )

    def parity(self, campaign, dat, label, mutated):
        """Relocation rejects whatever the exact verifiers reject, except
        exempt block functions of other recipes (mutated symbols given)."""
        result = campaign.relocate(dat, label)
        rejected = {entry["recipe"] for entry in result["rejected"]}
        exact = campaign.exact_rejects(dat, label)
        print(
            f"{campaign.name} {label}: {result['active']}/{result['total']} rejected "
            f"{len(rejected)} via {sorted({e['record'] for e in result['rejected']})}, "
            f"exact verifiers reject {len(exact)}",
            file=sys.stderr,
        )
        self.assertTrue(result["valid"])
        exemptions = campaign.exemptions()
        missed = sorted(
            hex(r) for r in exact - rejected if not mutated <= exemptions[r]
        )
        self.assertEqual(missed, [], f"{label}: exact verifiers reject more")
        return result, rejected, exact

    def test_verifier_reads_are_covered(self):
        for campaign in self.campaigns():
            gaps = campaign.coverage_gaps()
            print(
                f"{campaign.name} coverage: {len(campaign.recipes)} recipes, "
                f"{len(gaps)} gaps",
                file=sys.stderr,
            )
            self.assertEqual(gaps[:10], [])

    def test_review_mutations(self):
        tobacco = {
            "marcello": {0xB001, 0xB002, 0xB003},
            "dndr": {0x1B001, 0x1B002, 0x1B003},
        }
        for campaign in self.campaigns():
            cases = (
                (
                    "end_info_flasks",
                    campaign.call_amount_mutation(
                        "PC_POTIONALCHEMY_END_INFO", "CREATEINVITEMS", 2
                    ),
                ),
                ("name_value_text", campaign.text_mutation("NAME_VALUE")),
                ("tobacco_text", campaign.text_mutation("PRINT_TABAKSUCCESS")),
            )
            for label, case in cases:
                if case is None:
                    continue
                dat, mutated = case
                with self.subTest(campaign=campaign.name, mutation=label):
                    _, rejected, exact = self.parity(campaign, dat, label, mutated)
                    self.assertEqual(rejected, exact)
                    if label == "end_info_flasks" and campaign.name == "marcello":
                        self.assertLessEqual(tobacco["marcello"], rejected)
                    if label == "name_value_text" and campaign.name == "marcello":
                        self.assertEqual(rejected, set(range(0x9001, 0x9017)))
                    if label == "tobacco_text":
                        self.assertEqual(rejected, tobacco[campaign.name])

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
