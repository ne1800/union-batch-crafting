"""Name the missing translation keys and exercise single-file INI registration."""
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
TEXT = ROOT / "include/ubc/text"


def keys(table):
    return re.findall(r'\{\s*(Label::\w+|0x[0-9a-fA-F]+|\d+)\s*,\s*"', table)


class LanguageTables(unittest.TestCase):
    def test_all_files_cover_every_key(self):
        enum = (TEXT / "table.hpp").read_text().split("enum class Label {")[1].split("count\n};")[0]
        labels = {"Label::" + key for key in re.findall(r"\w+", enum)}
        reference = set(keys((TEXT / "locales/en.hpp").read_text()))
        for file in sorted((TEXT / "locales").glob("*.hpp")):
            with self.subTest(file=file.name):
                actual = keys(file.read_text())
                self.assertEqual({key for key in actual if key.startswith("Label::")}, labels)
                self.assertCountEqual(actual, reference)

    def test_one_new_file_is_discovered_and_selectable(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            shutil.copytree(TEXT, root / "include/ubc/text")
            shutil.copy(ROOT / "include/ubc/ui_language.hpp", root / "include/ubc")
            (root / "cmake").symlink_to(ROOT / "cmake", target_is_directory=True)
            (root / "CMakeLists.txt").write_text(
                "cmake_minimum_required(VERSION 3.21)\nproject(language_test LANGUAGES CXX)\n"
                "include(cmake/ui_languages.cmake)\nadd_executable(probe probe.cpp)\n"
                "target_include_directories(probe PRIVATE include)\ntarget_compile_features(probe PRIVATE cxx_std_17)\n")
            (root / "probe.cpp").write_text(
                '#include "ubc/ui_language.hpp"\n#include <string_view>\nint main() {\n'
                '  using namespace ubc::ui_text;\n  auto language = choose(L" QZ ", []{return true;});\n'
                '  return std::string_view(table(language)[Label::title]) == "Test translation" ? 0 : 1;\n}\n')
            build = root / "build"
            subprocess.run(["cmake", "-S", root, "-B", build, "-G", "Ninja"], check=True, stdout=subprocess.DEVNULL)
            locale = root / "include/ubc/text/locales/qz.hpp"
            source = (TEXT / "locales/en.hpp").read_text()
            locale.write_text(source.replace("locale_en", "locale_qz").replace("Union Batch Crafting", "Test translation"))
            subprocess.run(["cmake", "--build", build], check=True, stdout=subprocess.DEVNULL)
            subprocess.run([build / "probe"], check=True)
