"""Name the missing translation keys and exercise single-file INI registration."""

from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
TEXT = ROOT / "include/ubc/text"


def keys(table):
    return re.findall(r'\{\s*(Label::\w+|0x[0-9a-fA-F]+|\d+)\s*,\s*U"', table)


class LanguageTables(unittest.TestCase):
    def test_all_files_cover_every_key(self):
        enum = (
            (TEXT / "table.hpp")
            .read_text()
            .split("enum class Label {")[1]
            .split("count\n};")[0]
        )
        # Include labels from every build configuration, excluding the directives themselves.
        enum = re.sub(r"^[ \t]*#.*$", "", enum, flags=re.MULTILINE)
        labels = {"Label::" + key for key in re.findall(r"\w+", enum)}
        reference = set(keys((TEXT / "locales/en.hpp").read_text()))
        for file in sorted((TEXT / "locales").glob("*.hpp")):
            with self.subTest(file=file.name):
                actual = keys(file.read_text())
                self.assertEqual(
                    {key for key in actual if key.startswith("Label::")}, labels
                )
                self.assertCountEqual(actual, reference)

    def test_codepage_and_german_spelling(self):
        # Exact words with legitimate adjacent vowels or ss, including compounds.
        exceptions = set(
            """abgeschlossen aktuell ergebnisse essenz feuer feuerkraut
            feuerkräuter feuernessel feuernesseln feuerpfeil feuerpfeile feuerwurzel
            feuerwurzeln heilessenzen laborwasserflasche loslassen manaelixiere
            manaessenzen manaextrakte neue ordensschwert verbessert verlassen wasser zuerst""".split()
        )
        for file in (TEXT / "locales").glob("*.hpp"):
            source = file.read_text(encoding="utf-8")
            page = re.search(r"codepage = (\d+);", source)[1]
            for literal in re.findall(r'U"([^"]*)"', source):
                with self.subTest(file=file.name, text=literal):
                    literal.encode("cp" + page)
                    if file.stem == "de":
                        for word in re.findall(r"[^\W\d_]+", literal.lower()):
                            if re.search(r"ae|oe|ue|ss", word):
                                self.assertIn(word, exceptions)

    def test_one_new_file_is_discovered_and_selectable(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            shutil.copytree(ROOT / "include", root / "include")
            (root / "cmake").symlink_to(ROOT / "cmake", target_is_directory=True)
            (root / "CMakeLists.txt").write_text(
                "cmake_minimum_required(VERSION 3.21)\nproject(language_test LANGUAGES CXX)\n"
                "include(cmake/ui_languages.cmake)\nadd_executable(probe probe.cpp)\n"
                "target_include_directories(probe PRIVATE include)\ntarget_compile_features(probe PRIVATE cxx_std_17)\n"
            )
            shutil.copyfile(
                ROOT / "tests/native/text/language_probe.cpp", root / "probe.cpp"
            )
            build = root / "build"
            subprocess.run(
                ["cmake", "-S", root, "-B", build, "-G", "Ninja"],
                check=True,
                stdout=subprocess.DEVNULL,
            )
            locale = root / "include/ubc/text/locales/qz.hpp"
            source = (TEXT / "locales/en.hpp").read_text()
            locale.write_text(
                source.replace("locale_en", "locale_qz")
                .replace("codepage = 1252", "codepage = 1250")
                .replace("Union Batch Crafting", "Łódź")
            )
            subprocess.run(
                ["cmake", "--build", build], check=True, stdout=subprocess.DEVNULL
            )
            subprocess.run([build / "probe"], check=True)
