"""Check bilingual section order, language switches, link parity and local targets."""
from collections import Counter
from pathlib import Path
import re
import unittest
from urllib.parse import unquote, urlsplit

ROOT = Path(__file__).resolve().parents[1]
PAIRS = (("README.md", "README.de.md"), ("docs/STATIONS.md", "docs/STATIONEN.md"),
         ("INSTALL.md", "INSTALL.de.md"))
CANONICAL = {de: en for en, de in PAIRS}
HEADING = re.compile(r"^(#{1,6}) (.+)$", re.M)
ANCHOR = re.compile(r'<a id="([\w-]+)"></a>')
LINK = re.compile(r"\]\(([^)\s]+)\)")


def structure(root, filename):
    path = root / filename
    text = re.sub(r"^```[^\n]*\n.*?^```[^\n]*$", "", path.read_text(), flags=re.M | re.S)
    if re.search(r"^ {0,3}\[[^]\n]+\]:", text, re.M):
        raise ValueError(f"{filename}: use inline links so packaged evidence links can be resolved")
    headings = list(HEADING.finditer(text))
    own_pair = next(pair for pair in PAIRS if filename in pair)
    is_german = filename == own_pair[1]
    other_language = {en if is_german else de for en, de in PAIRS}
    sections = []
    for index, heading in enumerate(headings):
        before = text[:heading.start()].rstrip().splitlines()
        anchor = ANCHOR.fullmatch(before[-1]) if before else None
        if not anchor:
            raise ValueError(f"{filename}: heading needs a shared section ID: {heading[2]}")
        stop = headings[index + 1].start() if index + 1 < len(headings) else len(text)
        links = []
        for target in LINK.findall(text[heading.end():stop]):
            url = urlsplit(target)
            if url.scheme:
                links.append(target)
                continue
            dest = (path.parent / unquote(url.path)).resolve() if url.path else path
            if not dest.is_file():
                raise ValueError(f"{filename}: broken link {target}")
            relative = dest.relative_to(root).as_posix()
            fragment = unquote(url.fragment)
            if fragment:
                body = dest.read_text()
                slugs = {re.sub(r"[^\w -]", "", title.lower()).replace(" ", "-") for _, title in HEADING.findall(body)}
                if fragment not in slugs.union(ANCHOR.findall(body)):
                    raise ValueError(f"{filename}: missing anchor {target}")
            if index == 0 and relative in own_pair:
                links.append(relative)  # Check the two distinct language switches.
            else:
                if relative in other_language:
                    raise ValueError(f"{filename}: link opens the wrong language: {target}")
                if relative == "CHANGELOG.md" and fragment:
                    if fragment != ("deutsch" if is_german else "english"):
                        raise ValueError(f"{filename}: wrong changelog language: {target}")
                    fragment = "localized"
                links.append((CANONICAL.get(relative, relative), fragment))
        sections.append((heading[1], anchor[1], Counter(links)))
    return sections


class UserDocs(unittest.TestCase):
    def test_bilingual_sections_and_links(self):
        for en, de in PAIRS:
            with self.subTest(en=en):
                english = structure(ROOT, en)
                german = structure(ROOT, de)
                self.assertEqual([s[:2] for s in english], [s[:2] for s in german], "Section order or depth differs")
                self.assertEqual(len(english), len({s[1] for s in english}), "Repeated section ID")
                for sections in (english, german):
                    self.assertTrue(all(sections[0][2][name] == 1 for name in (en, de)), "Missing language switch")
                for left, right in zip(english, german, strict=True):
                    self.assertEqual(left[2], right[2], f"{en} / {de}: links differ in {left[1]}")
