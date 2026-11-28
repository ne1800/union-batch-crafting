<a id="player"></a>

# Union Batch Crafting 0.0.69 für Spieler

[English](INSTALL.md) · [Deutsch](INSTALL.de.md) · [Getestet mit](TESTED-WITH.md)

<a id="install"></a>

## Installieren

1. Installiere Union für Gothic II: Die Nacht des Raben. Beende Gothic und den Workshop-Launcher. Öffne in Steam die **Eigenschaften → Installierte Dateien → Durchsuchen**. Im richtigen Spielverzeichnis findest du `system/Gothic2.exe`.
2. Öffne `union-batch-crafting-player-0.0.69.zip`. Kopiere daraus `system/autorun/UnionBatchCraftingExtended.dll` in den entsprechenden Ordner im Spiel. Unter Linux/Proton übernimmst du die Schreibweise des vorhandenen autorun-Ordners. Vom Spielverzeichnis aus lautet der fertige Pfad `system/autorun/UnionBatchCraftingExtended.dll`. Liegt dort schon eine gleichnamige DLL, verschiebe sie vorher zur Sicherung aus dem Spielverzeichnis. Anleitungen und Lizenzen kannst du separat aufbewahren.
3. Starte Gothic wie gewohnt, wähle das Originalspiel oder Marcello L’Hiver DE 2.0.4 und starte oder lade ein Spiel. Wähle an einer unterstützten Station ein gelerntes Rezept, stelle die Menge ein und bestätige die Herstellung. Union lädt die DLL bei jedem Start aus dem autorun-Ordner.

Zum Ausprobieren nimm zwei Stück rohes Fleisch an einen Herd mit und stelle die Menge auf 2.

<a id="configure"></a>

## Konfigurieren

Beende Gothic. Die optionalen Einstellungen gehören in `system/Gothic.INI` neben `Gothic2.exe`. Ergänze den Abschnitt einmal oder ändere die vorhandenen Schlüssel. Änderungen gelten ab dem nächsten Spielstart.

```ini
[UNIONBATCHCRAFTING]
Enabled=1
Language=auto
```

`Enabled=0` schaltet das Plugin ab. Fehlt der Schlüssel oder steht er auf 1, ist es aktiv. Für das Mengenmenü erkennt `auto` deutsche Spieltexte und wählt sonst Englisch. Mit `de` oder `en` legst du die Sprache fest.

<a id="compatibility"></a>

## Unterstützte Fassungen und Stationen

Geprüft ist die deutsche Version 2.6 mit der mitgelieferten losen `_work/Data/Scripts/_compiled/GOTHIC.DAT` aus Gothic II: Gold Classic auf Steam (Workshop-Zweig, Build 9820952). Bei einer unbekannten DAT erscheint ein Kompatibilitätshinweis, wenn du Marcello L’Hiver gewählt hast. Andernfalls bleibt das Plugin untätig, auch bei anderen Sprach- und Patchständen von Die Nacht des Raben.

- **Die Nacht des Raben ohne Kampagnenmod** – Alchemietisch (20 Trank- und Sonderrezepte, drei Tabaksorten), Amboss (fünf normale und acht Erzwaffen), Esse, Pfanne und Herd.
- **Marcello L’Hiver DE 2.0.4** – 98 Rezeptprofile an Herd, Alchemietisch, Lederbank, Säge, Esse, Amboss, Bogenbank, Kühlbecken und Schleifstein.

In Die Nacht des Raben bieten Esse, Pfanne und Herd die Auswahl zwischen 1, freier Menge und Maximum. Jedes Stück durchläuft seine Originalanimation. Ein Abbruch im Mengenmenü ist kostenlos. Marvin-F8 nach der Reservierung kann wie im Original an der Esse einen kalten Rohstahl und am Herd ein rohes Fleisch kosten. Auch bei einem anderen harten Abbruch der Herdanimation kann das reservierte Rohfleisch verloren gehen.

Das Plugin prüft die Kompatibilität der Engine und die für das Rezept benötigten Skriptfunktionen und Symbole. Bei Abweichungen bleibt die Originalauswahl zugänglich. Ein abweichendes gemeinsames Engine-Layout kann das ganze Plugin abschalten. Bei geladenem Plugin findest du die Diagnose des aktuellen Starts in `system/autorun/UnionBatchCraftingExtended.log`. Der autorun-Ordner muss zum Anlegen des Logs beschreibbar sein. Frühere Spielnachweise und den Prüfumfang dieser Version nennt die [Kompatibilitätsliste](TESTED-WITH.md).

<a id="uninstall"></a>

## Deinstallieren

Beende Gothic und den Launcher. Entferne die installierte `system/autorun/UnionBatchCraftingExtended.dll` und, falls vorhanden, ihre `UnionBatchCraftingExtended.log`. Eine vorher gesicherte DLL kannst du jetzt zurückkopieren. Entferne bei Bedarf die beiden Einstellungen und den dann leeren Abschnitt `[UNIONBATCHCRAFTING]`. Behalte andere INI-Abschnitte, Union-Plugins und Spielstände. Die Herstellung verändert das Inventar im Spiel. Beim Speichern wird dieser Stand übernommen.
