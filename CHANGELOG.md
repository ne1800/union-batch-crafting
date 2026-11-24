# Changelog

[English](#english) · [Deutsch](#deutsch)

## English

- **Unreleased** The plugin stays idle when another campaign uses different scripts. Selecting Marcello L’Hiver with an unsupported script version still shows the compatibility notice. Campaign profiles now prepare a Night of the Raven alchemy pilot in development builds.

- **0.0.66** The grindstone preview now refreshes once when you open the menu and once per recipe, quantity or inventory change. A brief hitch of about 50 ms remains. In 0.0.65, opening the menu could cause repeated pauses, and a refresh could take a quarter of a second. If an ingredient is equipped, the recipe now shows `[equipped]` and asks you to unequip it first.

- **0.0.65** At the grindstone and other stations, the quantity menu reuses the preview while its values stay the same to reduce stutter. The grindstone can still stutter several times as the menu opens. Changing the quantity or recipe can cause a single pause of about a quarter of a second. Each time the menu opens, the plugin checks the game's menu functions. It recognizes the variant seen in the L’Hiver Workshop setup. If the check fails, you can still use the station's original controls.

- **0.0.64** Clearer detail lines explain returned tools, held items, zero-quantity previews, unknown stock counts, honing skill progress and bow material limits. If the menu window is rejected, no text is drawn to it and the original station controls remain usable.

- **0.0.63** A translucent black background makes the recipe list and quantity details easier to read. If the drawing code is unrecognized, the menu uses its text-only layout and logs the reason.

- **0.0.62** The quantity menu keeps one focus arrow. On wide screens, more recipes fit on a page, and `[x]` identifies the selected recipe while you adjust its quantity. The preview shows starting stock, output and final stock. The footer lists the keys for the active control.

- **0.0.61** Wrapped recipe names align with the text above them. Each recipe has the same spacing before the next entry.

- **0.0.60** Blocked recipes show the reason in the list. The craft action also shows a specific reason for the selected amount. An unknown sharpening recipe now fails the diagnostic check safely.

- **0.0.59** German menus, recipe names and item names now use umlauts and ß. The German quantity menu displayed “2 / 6 Durchgänge” with the expected Windows-1252 bytes while crafting two swamp tobaccos.

- **0.0.58** packages the in-game results with the byte-identical 0.0.57 player DLL.

- **0.0.57** splits the native crafting code into separate modules. The player and one-run test packages have been rebuilt. The player installation and an order of two swamp tobaccos passed the combined in-game test.

- **0.0.56** adds the permanent player ZIP, INI switch, scoped compatibility checks and bilingual install and removal instructions. The player installation awaits an in-game test.

- **0.0.55** documents tested recipes, quantities, module versions and removal of the one-run installation.
- **0.0.53–0.0.54** added automatic German menu text with English fallback, manual language selection, and bilingual guides.
- **0.0.52** accounts for apple tobacco and laboratory flasks when crafting refined tobacco.
- **0.0.51** added 20 bow and crossbow upgrade profiles.
- **0.0.45** verified the campfire's original meat quantities and cancellation.
- **0.0.43–0.0.44** enabled cooling the last two stored hot blades together and added diagnostics for rejected grinding orders.
- **0.0.30–0.0.35** measured and bounded the grinding preview. Rough swords passed the in-game comparison at 1, 2, 5 and maximum 3 in 0.0.35.

## Deutsch

- **Unveröffentlicht** Bei einer anderen Kampagne mit abweichenden Skripten bleibt das Plugin untätig. Ist Marcello L’Hiver mit einem nicht unterstützten Skriptstand gewählt, erscheint weiterhin der Fehlerhinweis. Kampagnenprofile bereiten einen DNdR-Alchemiepiloten für Entwicklungsbuilds vor.

- **0.0.66** Am Schleifstein wird die Vorschau beim Öffnen des Menüs und bei jedem Rezept-, Mengen- oder Bestandswechsel nur noch einmal berechnet. Ein kurzer Hänger von etwa 50 ms bleibt. In 0.0.65 konnte das Menü beim Öffnen mehrmals stocken, und eine Neuberechnung dauerte bis zu einer Viertelsekunde. Ist eine Zutat ausgerüstet, zeigt das Rezept jetzt `[ausgerüstet]` und bittet dich, sie zuerst abzulegen.

- **0.0.65** Am Schleifstein und an anderen Stationen verwendet das Mengenmenü die Vorschau weiter, solange ihre Werte gleich bleiben. Das verringert Ruckler. Beim Öffnen kann das Schleifsteinmenü noch mehrmals kurz hängen. Ein Mengen- oder Rezeptwechsel kann einen einzelnen Hänger von etwa einer Viertelsekunde auslösen. Bei jedem Öffnen prüft das Plugin die Menüfunktionen des Spiels. Es erkennt auch die Variante aus der L’Hiver-Workshop-Installation. Schlägt die Prüfung fehl, kannst du die Station wie im Original bedienen.

- **0.0.64** Verständlichere Detailzeilen erklären zurückgegebene Werkzeuge, gehaltene Gegenstände, Vorschauen bei Menge null, unbekannte Bestände, Schleifkunst und Materialgrenzen an der Bogenbank. Wird das Menüfenster abgewiesen, schreibt das Plugin keinen Text hinein und die Originalbedienung der Station bleibt bedienbar.

- **0.0.63** Ein halbtransparenter schwarzer Hintergrund macht die Rezeptliste und die Mengendetails besser lesbar. Bei einer unbekannten Zeichenroutine zeigt das Menü die bisherige Textansicht und protokolliert den Grund.

- **0.0.62** Im Mengenmenü zeigt ein Pfeil den Fokus. Auf breiten Bildschirmen passen mehr Rezepte auf eine Seite. Beim Einstellen der Menge bleibt das Rezept mit `[x]` markiert. Die Vorschau zeigt Anfangsbestand, Ausgabemenge und Endbestand als Rechnung. Die Fußzeile nennt die Tasten für das gewählte Feld.

- **0.0.61** Umgebrochene Rezeptnamen stehen bündig unter dem Text der ersten Zeile. Der Abstand zum nächsten Eintrag ist bei allen Rezepten gleich.

- **0.0.60** Gesperrte Rezepte zeigen den Grund in der Liste. Bei der Herstellung erscheint auch der konkrete Grund für die gewählte Menge. Die Diagnose behandelt unbekannte Schleifrezepte sicher.

- **0.0.59** Deutsche Menüs, Rezept- und Gegenstandsnamen verwenden jetzt Umlaute und ß. Bei der Herstellung von zwei Sumpftabaken zeigte das Mengenmenü „2 / 6 Durchgänge“ mit den erwarteten Windows-1252-Bytes.

- **0.0.58** enthält den Spielnachweis und die bytegleiche Spieler-DLL aus 0.0.57.

- **0.0.57** teilt den nativen Herstellungscode in einzelne Module auf. Spieler- und Einmal-Testpaket wurden neu gebaut. Die Spielerinstallation und ein Auftrag über zwei Sumpftabake bestanden den gemeinsamen Weltlauf.

- **0.0.56** bringt das dauerhaft aktive Spieler-ZIP, den INI-Schalter, abgestufte Versionsprüfungen und die zweisprachige Anleitung für Installation und Deinstallation. Der Spielnachweis mit diesem ZIP steht aus.

- **0.0.55** dokumentiert geprüfte Rezepte, Mengen, Modulstände und die Entfernung der Einmalinstallation.
- **0.0.53–0.0.54** brachten die automatische deutsche Menüanzeige mit englischem Rückfall, manuelle Sprachwahl und zweisprachige Anleitungen.
- **0.0.52** rechnet Apfeltabak und Laborflaschen bei der Herstellung von veredeltem Tabak ab.
- **0.0.51** ergänzte 20 Profile für die Verbesserung von Bögen und Armbrüsten.
- **0.0.45** bestätigte die originalen Fleischmengen und den Abbruch am Lagerfeuer.
- **0.0.43–0.0.44** ermöglichten das gemeinsame Kühlen der letzten beiden heißen Lagerklingen und ergänzten Diagnosen für abgelehnte Schleifaufträge.
- **0.0.30–0.0.35** erfassten und begrenzten den Aufwand für die Schleifvorschau. Grobe Schwerter bestanden in 0.0.35 den Weltvergleich mit 1, 2, 5 und Maximum 3.
