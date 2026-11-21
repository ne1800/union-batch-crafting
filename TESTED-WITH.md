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

Die Prüfsummen identifizieren die Referenz früherer Spieltests. Engine-Prüfstellen und Rezeptverträge bestimmen die verfügbaren Mengenpfade. Einzelne native Inventarpfade erfordern außerdem bestimmte Modullayouts.

Die Spieler-DLL aus 0.0.57 lud ohne Marker. Am Alchemietisch verbrauchte die Herstellung von zwei Sumpftabaken zwei Apfeltabake und zwei Laborwasserflaschen. Das Sumpfkraut blieb erhalten. Das Paket 0.0.58 enthält diesen Nachweis und dieselbe DLL.

Mit 0.0.59 zeigte das deutsche Mengenmenü „2 / 6 Durchgänge“ und „Zurück“. Das ä entsprach im Menüzustand dem erwarteten Windows-1252-Byte E4. Zwei Sumpftabake verbrauchten zwei Apfeltabake und zwei Laborwasserflaschen. Nach regulärem Beenden ohne Speichern und Deinstallation stimmten die SHA-256-Prüfsummen aller 410 Save-Dateien mit der Sicherung überein.

Mit 0.0.60 zeigte die deutsche Rezeptliste bei Lous Schnaps und permanenter Geschicklichkeit „[Zutaten fehlen]“. Beide Menüzeilen für permanente Geschicklichkeit entsprachen den erwarteten Windows-1252-Bytes. Zwei Sumpftabake verbrauchten zwei Apfeltabake und zwei Laborwasserflaschen. Nach dem Beenden ohne Speichern und der Deinstallation stimmten die Prüfsummen aller 410 Save-Dateien.

| Package component / Paketbestandteil | SHA-256 |
| --- | --- |
| Player DLL / Spieler-DLL (0.0.57, 0.0.58) | `54dd22377fc7eb2337fc8243f859666863f060e4f2bbb24a0e65b820ce0c2522` |
| In-game ZIP / ZIP im Weltlauf (0.0.57) | `512a11a159a38df4bf876f2f6d73e770e6bb50a4727c27fe1da8d0435d3cca03` |
| Player DLL / Spieler-DLL (0.0.59) | `dd530e2df60b59866a97c05aa10609327d2f374c8895330af36ef689b1e02003` |
| In-game ZIP / ZIP im Weltlauf (0.0.59) | `b61a119295d40eee24e9e95e98e4ce3929550e17d30a4935b251706f723b9263` |
| Player DLL / Spieler-DLL (0.0.60) | `5d702a8b9310ad52975674ee6febf3bcd7157daac9242819e0261e435abe68e3` |
| In-game ZIP / ZIP im Weltlauf (0.0.60) | `26a106b6e843917b0871fbe86c400b2af631ddf09368ca1a3ebf4ade6ffa7023` |
