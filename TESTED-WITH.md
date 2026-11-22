# Tested with · Getestet mit

Gothic II: Night of the Raven / Die Nacht des Raben, Windows x86, 2.6 fix, Marcello L’Hiver **DE 2.0.4**, Union **1.0m**.

| File / Datei | Tested SHA-256 / geprüfter SHA-256 |
| --- | --- |
| Gothic2.exe | `0655b4811e008a353ce372fde463ef00b6101f3e89be850e2f14980140447c7f` |
| Shw32.dll (Union) | `78d9312f07ebdf978d301fde63841ddcbe265b59cb7c2753e01d2dbdc5742888` |
| zBugFixes.dll | `50c3dc669a451dc084c4691b6e6fb7a39838d40627629e59e884579873811963` |
| UNION_VOB_LOCATOR.DLL (New Vob Locator) | `72fc040ffe3d9ee6995493d4e5879336bbccf2f2301f5daa4578ff8f31b234fc` |

These fingerprints identify the earlier in-game reference. Engine sites and recipe contracts determine which crafting paths are available. Some native inventory paths also require specific module layouts.

The player DLL from 0.0.57 loaded without a marker. At the alchemy table, selecting two swamp tobaccos used two apple tobaccos and two laboratory flasks. Swampweed was unchanged. Package 0.0.58 includes this result and the same DLL.

In 0.0.59, the German quantity menu displayed “2 / 6 Durchgänge” and “Zurück”. The game used the expected Windows-1252 byte E4 for ä. Crafting two swamp tobaccos consumed two apple tobaccos and two laboratory flasks. After a normal exit without saving and uninstallation, all 410 save files matched their original SHA-256 hashes.

In 0.0.60, the German recipe list showed “[Zutaten fehlen]” for Lou’s booze and permanent dexterity. The displayed bytes for permanent dexterity matched the expected Windows-1252 lines. Two swamp tobaccos consumed two apple tobaccos and two laboratory flasks. All 410 save files matched after exiting without saving and uninstalling the DLL.

In 0.0.61, all ten lines of the six visible German recipes matched the expected coordinates and Windows-1252 bytes at the alchemy table. The screenshot shows the hanging indent. Two swamp tobaccos used two apple tobaccos and two laboratory flasks. After exiting without saving and uninstalling the player DLL, the hashes of all 410 save files and 21 INI and autorun files matched the fresh backup.

In 0.0.62, all twelve known recipes in slot 14 fit on one page at the alchemy table. The focus arrow marked the quantity while `[x]` marked swamp tobacco. The preview showed `2 + 2 = 4` and crafting produced that stock. Apple tobacco went from 6 to 4, laboratory flasks from 26 to 24, and swampweed stayed at 116. All 410 save files matched the fresh backup after exit and uninstallation.

Die Prüfsummen identifizieren die Referenz früherer Spieltests. Engine-Prüfstellen und Rezeptverträge bestimmen die verfügbaren Mengenpfade. Einzelne native Inventarpfade erfordern außerdem bestimmte Modullayouts.

Die Spieler-DLL aus 0.0.57 lud ohne Marker. Am Alchemietisch verbrauchte die Herstellung von zwei Sumpftabaken zwei Apfeltabake und zwei Laborwasserflaschen. Das Sumpfkraut blieb erhalten. Das Paket 0.0.58 enthält diesen Nachweis und dieselbe DLL.

Mit 0.0.59 zeigte das deutsche Mengenmenü „2 / 6 Durchgänge“ und „Zurück“. Das ä entsprach im Menüzustand dem erwarteten Windows-1252-Byte E4. Zwei Sumpftabake verbrauchten zwei Apfeltabake und zwei Laborwasserflaschen. Nach regulärem Beenden ohne Speichern und Deinstallation stimmten die SHA-256-Prüfsummen aller 410 Save-Dateien mit der Sicherung überein.

Mit 0.0.60 zeigte die deutsche Rezeptliste bei Lous Schnaps und permanenter Geschicklichkeit „[Zutaten fehlen]“. Beide Menüzeilen für permanente Geschicklichkeit entsprachen den erwarteten Windows-1252-Bytes. Zwei Sumpftabake verbrauchten zwei Apfeltabake und zwei Laborwasserflaschen. Nach dem Beenden ohne Speichern und der Deinstallation stimmten die Prüfsummen aller 410 Save-Dateien.

Mit 0.0.61 stimmten am Alchemietisch die Koordinaten und Windows-1252-Bytes aller zehn Zeilen der sechs sichtbaren Rezepte. Das Bildschirmfoto zeigt die eingerückten Folgezeilen. Zwei Sumpftabake verbrauchten zwei Apfeltabake und zwei Laborwasserflaschen. Nach dem Beenden ohne Speichern und dem Ausbau der Spieler-DLL entsprachen die Prüfsummen aller 410 Save-Dateien und 21 INI- und Autorun-Dateien der frischen Sicherung.

Mit 0.0.62 passten am Alchemietisch in Slot 14 alle zwölf bekannten Rezepte auf eine Seite. Der Fokuspfeil stand bei der Menge, `[x]` beim Sumpftabak. Die Vorschau zeigte `2 + 2 = 4`, das Inventar bestätigte vier Sumpftabake. Apfeltabak sank von 6 auf 4, Laborwasserflaschen von 26 auf 24. Der Sumpfkrautbestand blieb bei 116. Nach Spielende und Ausbau entsprachen alle 410 Save-Dateien der frischen Sicherung.

| Package component / Paketbestandteil | SHA-256 |
| --- | --- |
| Player DLL / Spieler-DLL (0.0.57, 0.0.58) | `54dd22377fc7eb2337fc8243f859666863f060e4f2bbb24a0e65b820ce0c2522` |
| In-game ZIP / ZIP im Weltlauf (0.0.57) | `512a11a159a38df4bf876f2f6d73e770e6bb50a4727c27fe1da8d0435d3cca03` |
| Player DLL / Spieler-DLL (0.0.59) | `dd530e2df60b59866a97c05aa10609327d2f374c8895330af36ef689b1e02003` |
| In-game ZIP / ZIP im Weltlauf (0.0.59) | `b61a119295d40eee24e9e95e98e4ce3929550e17d30a4935b251706f723b9263` |
| Player DLL / Spieler-DLL (0.0.60) | `5d702a8b9310ad52975674ee6febf3bcd7157daac9242819e0261e435abe68e3` |
| In-game ZIP / ZIP im Weltlauf (0.0.60) | `26a106b6e843917b0871fbe86c400b2af631ddf09368ca1a3ebf4ade6ffa7023` |
| Player DLL / Spieler-DLL (0.0.61) | `ce7dd2bb779e9357ba8dce85daa593c45c084ee20a294060f56bad44ca756c04` |
| In-game ZIP / ZIP im Weltlauf (0.0.61) | `a872e792a4ae7e5f1e9e5f96d6e7bd8ada6458fd876aa2ed1c3af2735ccb9ed3` |
| Player DLL / Spieler-DLL (0.0.62) | `cf9af3d86e2bb1e7f3b7cdc241fd8759efc63d512b1d2f72d16a38e21e425f11` |
| In-game ZIP / ZIP im Weltlauf (0.0.62) | `be80758b71518ec0e40f3925ccd48b8a3c7c3938480addb9bd6948fbfdae151c` |

The release ZIP also contains this file, so its SHA-256 differs from the ZIP used in game. Compare the player DLL instead. Each release lists the SHA-256 of its ZIP and DLL. · Das Release-ZIP enthält auch diese Datei, sein SHA-256 weicht deshalb vom ZIP im Weltlauf ab. Vergleiche stattdessen die Spieler-DLL. Jedes Release nennt den SHA-256 seines ZIPs und seiner DLL.
