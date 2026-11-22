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

In 0.0.63, the alchemy table and the outdoor saw at night both showed a translucent black background at 160/255 opacity. The recipe names, quantities and controls were legible. Crafting two swamp tobaccos in slot 14 raised the stock from 2 to 4 and used two apple tobaccos and two flasks. Returning to the original saw controls preserved all 418 inventory counts. All 410 save files matched the backup after exit and uninstallation.

In a separate 0.0.63 run, the same player DLL processed seven orders at the leather bench in one load of slot 14. Each used two hides. Sheep, lurker, boar and warg each yielded 2 leather, swampshark 4, shadowbeast 6 and troll 8. Leather rose from 8 to 34. All 418 inventory counts matched the expected balance after each order. After a normal exit without saving and removal of the test installation, all 410 save files matched the backup.

At the bow bench, each order with the 0.0.63 player DLL started from a fresh load of slot 14. Orders of two succeeded for the fine bow, hunting bow, crossbow and fine crossbow. Orders of one succeeded for the composite bow, ash bow and longbow. The two fine crossbows used two base weapons, eight raw steel and all eight leather. The complete inventory matched after every order. All 410 save files matched the backups after exit without saving and removal of the test DLL.

Die Prüfsummen identifizieren die Referenz früherer Spieltests. Engine-Prüfstellen und Rezeptverträge bestimmen die verfügbaren Mengenpfade. Einzelne native Inventarpfade erfordern außerdem bestimmte Modullayouts.

Die Spieler-DLL aus 0.0.57 lud ohne Marker. Am Alchemietisch verbrauchte die Herstellung von zwei Sumpftabaken zwei Apfeltabake und zwei Laborwasserflaschen. Das Sumpfkraut blieb erhalten. Das Paket 0.0.58 enthält diesen Nachweis und dieselbe DLL.

Mit 0.0.59 zeigte das deutsche Mengenmenü „2 / 6 Durchgänge“ und „Zurück“. Das ä entsprach im Menüzustand dem erwarteten Windows-1252-Byte E4. Zwei Sumpftabake verbrauchten zwei Apfeltabake und zwei Laborwasserflaschen. Nach regulärem Beenden ohne Speichern und Deinstallation stimmten die SHA-256-Prüfsummen aller 410 Save-Dateien mit der Sicherung überein.

Mit 0.0.60 zeigte die deutsche Rezeptliste bei Lous Schnaps und permanenter Geschicklichkeit „[Zutaten fehlen]“. Beide Menüzeilen für permanente Geschicklichkeit entsprachen den erwarteten Windows-1252-Bytes. Zwei Sumpftabake verbrauchten zwei Apfeltabake und zwei Laborwasserflaschen. Nach dem Beenden ohne Speichern und der Deinstallation stimmten die Prüfsummen aller 410 Save-Dateien.

Mit 0.0.61 stimmten am Alchemietisch die Koordinaten und Windows-1252-Bytes aller zehn Zeilen der sechs sichtbaren Rezepte. Das Bildschirmfoto zeigt die eingerückten Folgezeilen. Zwei Sumpftabake verbrauchten zwei Apfeltabake und zwei Laborwasserflaschen. Nach dem Beenden ohne Speichern und dem Ausbau der Spieler-DLL entsprachen die Prüfsummen aller 410 Save-Dateien und 21 INI- und Autorun-Dateien der frischen Sicherung.

Mit 0.0.62 passten am Alchemietisch in Slot 14 alle zwölf bekannten Rezepte auf eine Seite. Der Fokuspfeil stand bei der Menge, `[x]` beim Sumpftabak. Die Vorschau zeigte `2 + 2 = 4`, das Inventar bestätigte vier Sumpftabake. Apfeltabak sank von 6 auf 4, Laborwasserflaschen von 26 auf 24. Der Sumpfkrautbestand blieb bei 116. Nach Spielende und Ausbau entsprachen alle 410 Save-Dateien der frischen Sicherung.

Mit 0.0.63 war der halbtransparente schwarze Hintergrund am Alchemietisch und nachts an der Holzsäge im Freien mit Deckkraft 160/255 sichtbar. Rezeptnamen, Mengen und Bedienhinweise waren lesbar. Zwei Sumpftabake in Slot 14 erhöhten den Bestand von 2 auf 4 und verbrauchten zwei Apfeltabake und zwei Flaschen. Nach der Rückkehr zur Originalbedienung an der Säge stimmten alle 418 Inventarbestände. Nach Spielende und Ausbau waren alle 410 Save-Dateien zur Sicherung bytegleich.

In einem weiteren Lauf mit derselben Spieler-DLL 0.0.63 wurden nach einmaligem Laden von Slot 14 sieben Aufträge an der Lederbank ausgeführt. Jeder verbrauchte zwei Felle oder Häute. Schaf, Lurker, Keiler und Warg ergaben je 2 Leder, Sumpfhai 4, Schattenläufer 6 und Troll 8. Der Ledervorrat stieg von 8 auf 34. Nach jedem Auftrag stimmten alle 418 Inventarbestände mit der erwarteten Bilanz überein. Nach regulärem Beenden ohne Speichern und Ausbau der Testinstallation waren alle 410 Save-Dateien zur Sicherung bytegleich.

An der Bogenbank begann jeder Auftrag mit der Spieler-DLL 0.0.63 aus einer frischen Ladung von Slot 14. Guter Bogen, Jagdbogen, Armbrust und gute Armbrust gelangen in Menge 2, Komposit-, Eschen- und Langbogen in Menge 1. Die beiden guten Armbrüste verbrauchten zwei Ausgangswaffen, acht Rohstahl und alle acht Leder. Nach jedem Auftrag stimmte die vollständige Inventarbilanz. Nach dem Beenden ohne Speichern und dem Ausbau der Test-DLL waren alle 410 Save-Dateien zur jeweiligen Sicherung bytegleich.

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
| Player DLL / Spieler-DLL (0.0.63) | `16246460d0e77a66a454e50c6676db94765fdbd3c4d2f27b70f62d80a92e2b6a` |
| In-game ZIP / ZIP im Weltlauf (0.0.63) | `ff0e3545f377e18b447b4f67e6f979d42b8eb69d850acf34a7412ab53faaf0b4` |
| Leather and bow bench ZIP / ZIP der Leder- und Bogenbankläufe (0.0.63) | `22595581ecb07cfe5ea48501a09bde00571622a9f0676d9802df70eee1965ccb` |

Each ZIP hash identifies one archive, including its documentation. Use the player DLL hash to match a release to an in-game test. Each release lists both hashes. · Der ZIP-Hash kennzeichnet ein einzelnes Archiv einschließlich seiner Dokumentation. Mit dem Hash der Spieler-DLL lässt sich ein Release einem Weltlauf zuordnen. Jedes Release nennt beide Prüfsummen.
