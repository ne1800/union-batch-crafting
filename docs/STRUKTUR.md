<a id="structure"></a>

# Quellstruktur

[English](STRUCTURE.md) · [Deutsch](STRUKTUR.md)

| Ordner | Aufgabe |
| --- | --- |
| `src/plugin/` | Hier liegen DLL-Einstieg, Ressourcen und die Windows-Anbindung der Herstellung. |
| `src/plugin/crafting/` | Die Übersetzungseinheiten teilen sich den Stationszustand und verarbeiten Dialog, Mengenwahl, Buchung und Abschluss. |
| `src/bridges/` | Diese Adapter rufen Engine-Funktionen für Frames, Mengen, Munition und Materialverbrauch auf. |
| `src/writers/` | Diese Routinen führen die geprüften Bestandsänderungen für Alchemie, Geisttrank und Tabak aus. |
| `src/ui/` | Diese Adapter verbinden die Menümodelle und die Sprachwahl mit der nativen Oberfläche. |
| `src/probes/` | Die sechs separaten Diagnose-Plugins haben eigene Einstiegspunkte und Exportdefinitionen für Testhosts und Diagnosepakete. |
| `include/ubc/catalogs/` | Hier liegen Rezeptdeskriptoren, Anzeigenamen und die zugehörigen Profiltabellen. |
| `include/ubc/core/` | Hier liegen stationsübergreifende Regeln für Mengen, Auswahl, Rezeptaufträge und Ergebnisse sowie SHA-256. |
| `include/ubc/engine/` | Diese Header beschreiben Engine-Schnittstellen, Speicherleser, Modulprüfungen und Aufrufstellen. |
| `include/ubc/stations/` | Die gemeinsamen Stationsverträge liegen direkt hier, die Besonderheiten der einzelnen Stationen in den Unterordnern. |
| `stations/alchemy/` | Die Alchemie-Leser prüfen Zusatzrezepte, Abhängigkeiten und Flaschenverbrauch. |
| `stations/anvil/` | Die Ambosslogik prüft Rezeptwahl, Bestände und Dialogablauf. |
| `stations/bow/` | Die Bogenbank verbindet Rezeptauswahl und Gegenstandsprüfung. |
| `stations/cool/` | Die Kühlwanne prüft Klingen, Vorabverbrauch und Stationsende. |
| `stations/grind/` | Der Schleifstein prüft Waffenprofile, Vorschau, Verbrauch und Ergebnis. |
| `stations/ore/` | Die Erzrezepte besitzen eigene Auswahl-, Laufzeit- und Bestandsprüfungen. |
| `stations/saw/` | Die Säge verarbeitet den Stationszustand und die Munitionsausgabe. |
| `stations/spirit/` | Der Geisttrank prüft Zutaten, Ausgabezweig und Flaschenverbrauch. |
| `stations/tobacco/` | Die Tabakrezepte prüfen den Parserzustand und die Mengenbuchung. |
| `include/ubc/ui/` | Hier liegen die Menümodelle und Schnittstellen der nativen Oberfläche. |
| `include/ubc/text/` | Hier liegen Sprachwahl, Textschlüssel und Tabellenformat. |
| `include/ubc/text/locales/` | Jede unterstützte Sprache definiert hier ihre vollständige Übersetzungstabelle. |

Die nativen Tests folgen unter `tests/native/` den Headerbereichen. Gemeinsame C++-Testhelfer stehen in `tests/native/helpers/`. Die Python-Tests unter `tests/python/` folgen den Werkzeugordnern. Werkzeuge für Build, Paketerstellung und Tests liegen in `tools/build/`, `tools/package/` und `tools/test/`. Die Hook-Skripte liegen in `tools/lint/`.

Die mise-Aufgaben folgen denselben Zuständigkeiten. `package:probe-WRITES` baut das Diagnosepaket, `package:player-WRITES` das Spielerpaket. `lint:all` prüft die versionierten Dateien.

<a id="tables"></a>

## Pflege der Rezepttabellen

Die eingecheckten Kataloge sind die Quelle für den Build. Der Bogenkatalog wird direkt im Header gepflegt, etwa bei Korrekturen der gemessenen Laufzeitnamen. Die Tabelle im Schleifprofil sowie die zugehörigen Gegenstandsformen und Konstruktordaten stammen aus einer früheren Offline-Auswertung. Ihre Header nennen den Erzeugerbefehl für die damalige Arbeitsumgebung. Der Test `recipe_family_table` vergleicht die 98 kompilierten Profile mit den eingecheckten Referenzzeilen. [Profilvergleich](../tests/native/catalogs/recipe_family_table_tests.cpp)
