<a id="overview"></a>

# Union Batch Crafting

[English](README.md) · [Deutsch](README.de.md)

Die deutschen Menüs zeigen Umlaute und ß. Das Spieler-ZIP **0.0.59** bleibt nach der Installation aktiv. In Gothic.INI schaltet `Enabled=0` es ab. [Spielerinstallation und Deinstallation](INSTALL.de.md)

Wähle in Gothic II: Die Nacht des Raben mit Marcello L’Hiver DE 2.0.4 und Union ein Rezept und stelle mehrere Portionen auf einmal her. Version 0.0.57 deckt 98 Profile an neun Stationen ab. Die Anleitung nennt die im Spiel geprüften Mengen und erklärt freie Mengenwahl, Abbruch und den Prüfstand zur Controllerbedienung. [Stationsanleitung](docs/STATIONEN.md) · [Changelog](CHANGELOG.md#deutsch)

<a id="build"></a>

## Aus dem Quellcode bauen

Unter Linux brauchst du Clang mit C++17-Unterstützung, CMake ab 3.21, Ninja, Python ab 3.10 und mise. Baue und prüfe den portablen Teil im Stammverzeichnis dieses Checkouts:

```sh
mise trust
mise run build:native
ctest --test-dir build/native --output-on-failure
mise run test:tools
```

Die Python-Tests prüfen beide Sprachfassungen und ihre Links. Die nativen Tests nutzen synthetische Testdaten und die Rezepttabelle. Die positiven Prüfungen gegen Original-EXE und Modarchiv benötigen eine passende lokale Spielinstallation und werden beim portablen Lauf übersprungen.

Für den Windows-Bau brauchst du zusätzlich clang-cl, LLD, llvm-rc, llvm-mt und das x86-SDK mit CRT. Wenn du Microsofts SDK-Lizenz akzeptierst, lädt der optionale erste Befehl die festgelegte xwin-Version und richtet das SDK unter `.tools/xwin` ein. Liegt es dort bereits vor, genügt der zweite Befehl.

```sh
mise run setup:windows-WRITES -- --apply
mise run build:windows
```

Das Plugin liegt unter `build/windows/player/UnionBatchCraftingExtended.dll`. Der Bau benötigt keine Spieldateien. Der eigene Code steht unter der [MIT-Lizenz](LICENSE). Für xwin gilt MIT/Apache-2.0. Für das SDK und die statisch in die Windows-DLL eingebundene Release-CRT gelten Microsofts Bedingungen.

Das Spieler-ZIP entsteht mit `mise run package:player-WRITES -- --apply`.

<a id="development"></a>

## Entwicklung

`mise run lint:all` führt prek auf allen versionierten Dateien aus. prek installiert beim ersten Lauf alle Formatierer und Linter in fest gepinnten Versionen, darunter clang-format 23.1.5 und gersemi 0.29.2. Mit `mise run test:report-WRITES -- --apply` laufen die portablen Tests. Git-Hooks werden im privaten Entwicklungs-Checkout mit `mise x -- prek install` für pre-commit und commit-msg aktiviert. Im Export laufen die Prüfungen über die mise-Aufgaben.

Commit-Betreffzeilen sind auf Englisch und folgen Conventional Commits, etwa `fix(ui): handle empty recipe lists`. Der Scope ist optional. Ein `!` vor dem Doppelpunkt kennzeichnet eine inkompatible Änderung.
