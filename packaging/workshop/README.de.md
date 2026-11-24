# Union Batch Crafting im Steam Workshop veröffentlichen

[English](README.md)

## Addon bauen

Im Quellcheckout mit eingerichtetem Windows-Bauwerkzeug die gepinnte Spieler-DLL bauen und verpacken. Der letzte Befehl liest das Archiv zurück, vergleicht den SHA-256 mit `tools/ci/dll.sha256` und prüft beide Lizenztexte.

```sh
mise run build:windows -- --target UnionBatchCraftingPlayer
mise run package:workshop-WRITES -- --apply
mise run package:verify-workshop
```

Der Inhaltsordner enthält eine Datei, `Data/UnionBatchCrafting.vdf`. Unter dem virtuellen Pfad `SYSTEM/AUTORUN` liegt die Spieler-DLL. Unter `DOCS/UNIONBATCHCRAFTING` liegen `LICENSE` und `THIRD-PARTY-NOTICES`. So erhalten Abonnenten die Lizenzhinweise mit der DLL und die Texte bleiben außerhalb von Autorun. Der Packer verwendet eine feste Katalogreihenfolge und den DOS-Zeitstempel 01.01.2000. Der Launcher passt den Zeitstempel im Kopf der installierten Kopie an die Ladereihenfolge an.

Für den Formatvergleich mit dem installierten ScreenInfo-Plugin (Workshop-Eintrag 2787243474) `UBC_SCREENINFO_VDF` auf den vollständigen Pfad seiner VDF setzen und die Pakettests im Checkout starten.

```sh
mise x -- python3 -B -m unittest discover -s tests/python/package -p test_workshop.py
```

## Steam und Vorschaubild vorbereiten

Steam auf dem Desktop mit dem Konto öffnen, dem der Eintrag gehören soll. Den Steam-Nutzungsvertrag und die gegebenenfalls angeforderten Workshop-Bedingungen akzeptieren. In der Gothic-II-Workshop-Installation müssen der Uploader und `steam_appid.txt` unter `launcher` liegen. Den physischen Controller im Spiel testen. Vor der Veröffentlichung das Vorschaubild (500 × 500, JPG oder PNG) freigeben und die Rechte an den dargestellten Spiel- und Modinhalten prüfen. Das Bild liegt außerhalb von `content`.

Diese Variablen in **Bash** im Quellcheckout setzen. Beide Beispielpfade ersetzen und die Shell für den gewünschten Vorgang geöffnet lassen. Das standardmäßige Wine-Laufwerk `Z:` bildet das Linux-Dateisystem ab. Der Befehl startet den Uploader aus dem Verzeichnis mit der Steam-App-ID.

```bash
repo=$(pwd -P)
game='/path/to/SteamLibrary/steamapps/common/Gothic II'
art='/path/to/approved-preview.jpg'
meta="$repo/packaging/workshop"
winpath() { local p; p=$(realpath -e -- "$1") || return; printf 'Z:%s\n' "${p//\//\\}"; }
content=$(winpath "$repo/dist/workshop/content") && preview=$(winpath "$art") || exit 1
cd -- "$game/launcher" || exit 1
```

## Neuen Eintrag anlegen

Einmal ausführen, wenn der versteckte Eintrag angelegt werden soll. Der Befehl übergibt den englischen Titel und die englische BBCode-Beschreibung als separate Argumente.

```bash
protontricks-launch --appid 39510 ./GothicSteamWorkshopUploader.exe publish \
  --game=g2 --title="$(cat "$meta/title.en.txt")" \
  --description="$(cat "$meta/description.en.bbcode")" --tags="$(cat "$meta/tags.txt")" \
  --content="$content" --preview="$preview"
```

Auf der versteckten Workshop-Seite Union (2787458679) und Marcello L’Hiver DE (2847787799) als **benötigte Objekte** eintragen. In der Sprachverwaltung den deutschen Titel aus `title.de.txt` und die Beschreibung aus `description.de.bbcode` ergänzen. Beide Sprachen verwenden dasselbe Addon. Den versteckten Eintrag abonnieren, mit Union aktivieren, Marcello L’Hiver DE 2.0.4 auswählen und im Spiel prüfen. Anschließend kann der Eintrag sichtbar geschaltet werden. Nach einer früheren manuellen Installation zuerst Gothic schließen und die lose `system/autorun/UnionBatchCraftingExtended.dll` entfernen.

## Bestehenden Eintrag aktualisieren

Nach dem Neubau und der Archivprüfung die ID des bestehenden Workshop-Eintrags einsetzen und die tatsächliche Änderung beschreiben. Das Beispiel aktualisiert den Inhalt. Für ein neues freigegebenes Vorschaubild zusätzlich `--preview="$preview"` angeben.

```bash
item_id='<ID>'
change='<changenote>'
protontricks-launch --appid 39510 ./GothicSteamWorkshopUploader.exe update \
  --game=g2 --mod="$item_id" --changenote="$change" --content="$content"
```

## Lokale Quellen

Die mitgelieferte `launcher/GothicSteamWorkshopGuide.pdf` beschreibt Addon-Tag, Verzeichnisaufbau, Bildmaße, versteckte Sichtbarkeit, Abhängigkeiten und beide Uploader-Befehle. Die Workshop-Regeln verlangen eine fertige, geprüfte, nichtkommerzielle Veröffentlichung unter Beachtung der Rechte Dritter. Die installierte `launcher/example_mod/publish.bat` zeigt `--game=g2` sowie die getrennten Pfade für Inhalt und Vorschau. Die lokale Hilfe `protontricks-launch --help` belegt den Aufruf mit `--appid APPID executable [args]`. Die Befehle wurden offline mit diesen Quellen abgeglichen. Veröffentlichen und Aktualisieren erfordern die laufende Steam-Sitzung des Inhabers.
