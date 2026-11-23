<a id="guide"></a>

# Union Batch Crafting

[English](STATIONS.md) · [Deutsch](STATIONEN.md)

In Marcello L’Hiver DE 2.0.4 unterstützt das Herstellungsmenü 98 Rezeptprofile an Herd, Alchemietisch, Lederbank, Säge, Esse, Amboss, Bogenbank, Kühlbecken und Schleifstein. Das Menü zeigt gelernte Rezepte, Bedarf, Vorrat und die Vorschau für die gewählte Menge.

<a id="install"></a>

## Installation für Spieler

Die Installation des Spieler-ZIPs aktiviert das Plugin. Zum Abschalten setze `Enabled=0` in der `system/Gothic.INI`. [Spielerinstallation und Deinstallation](../INSTALL.de.md)

<a id="compatibility"></a>

## Getestet mit

Die Weltvergleiche gelten für Gothic II DNdR, Marcello L’Hiver DE 2.0.4 und die folgenden geladenen Dateien. Die SHA-256-Werte identifizieren die getesteten Stände. Nach einem Modulupdate gelten die früheren Weltbelege weiterhin für ihre ursprüngliche Kombination.

| Modul | SHA-256 |
| --- | --- |
| Union 1.0m · Shw32.dll | `78d9312f07ebdf978d301fde63841ddcbe265b59cb7c2753e01d2dbdc5742888` |
| zBugFixes.dll | `50c3dc669a451dc084c4691b6e6fb7a39838d40627629e59e884579873811963` |
| New Vob Locator · UNION_VOB_LOCATOR.DLL | `72fc040ffe3d9ee6995493d4e5879336bbccf2f2301f5daa4578ff8f31b234fc` |
| Screen Info · UNION_GOTHIC_SCREEN_INFO.DLL | `c311eb0e3ef2416f4ba6f9c7beed7d1e01692622cdaa97e0ee0120ee086fc236` |

`UNION_GOTHIC_SCREEN_INFO.DLL` patcht `zCMenu::ScreenInit`. Das Plugin lässt die getestete Variante beim Öffnen des Mengenmenüs und der Herstellungsquittung zu.

<a id="controls"></a>

## Bedienung

Öffne eine angebundene Station. Wähle mit Auf/Ab ein bekanntes Rezept. Mit Tab wechselst du den Bereich, mit Links/Rechts die Menge um eins. Für größere Mengen gibt es −5/+5 sowie 1/5/10/Maximum. Prüfe Bedarf und Vorschau, wähle „Herstellen“ und bestätige mit A/Enter. B/Escape geht zurück. Über „Originale Bedienung“ erreichst du die ursprüngliche Auswahl. Die vorhandene Controllerbelegung bleibt nutzbar. Die physische Bedienung an Stationen ist noch offen.

Der Pfeil `>` zeigt das aktive Feld. Beim Einstellen der Menge markiert `[x]` das gewählte Rezept in der Liste. Bild auf erhöht die Menge um fünf, Bild ab verringert sie um fünf. Eine Zutat aus der Hand zählt zur angezeigten Verbrauchsmenge.

Bei gesperrten Rezepten steht der Grund in eckigen Klammern, etwa „[Zutaten fehlen]“ oder „[Platz fehlt]“. Auch bei „Herstellen“ erscheint der Grund für die gewählte Menge, sobald er bekannt ist. Während der Prüfung steht dort „[gesperrt]“.

<a id="configuration"></a>

## Konfiguration und Sprachwahl

In `system/Gothic.INI` steuert dieser optionale Abschnitt die Sprache der Plugin-Menüs:

```ini
[UNIONBATCHCRAFTING]
Enabled=1
Language=auto
```

Bei `auto`, einem leeren Wert oder einem fehlenden Eintrag erkennt das Plugin deutsche Spieltexte. Schlägt die Erkennung fehl, erscheint Englisch. Mit `de` erzwingst du Deutsch, mit `en` Englisch. Unbekannte Werte ergeben Englisch. Die Sprache der Originaldialoge bestimmt das Spiel.

<a id="adding-a-language"></a>

## Eine Spielsprache hinzufügen

1. Kopiere im Quellrepository `include/ubc/text/locales/en.hpp` im selben Verzeichnis nach beispielsweise `fr.hpp`. Verwende einen zweibuchstabigen Kleinbuchstabencode und ändere in dieser Kopie den Namensraum zu `locale_fr`.
2. Übersetze die Texte in `messages`, `items` und `recipes`. Behalte jeden Schlüssel, die numerisch sortierten IDs und die Reihenfolge und Typen der printf-Platzhalter bei. Speichere die Datei als UTF-8 und schreibe die Zeichen direkt in die Unicode-Literale. Setze `codepage` auf die Windows-Codepage des Spiels (1252 für Deutsch und Englisch, 1250 für Polnisch, 1251 für Russisch). Der Build erzeugt pro Zeichen ein Spielbyte und meldet nicht darstellbare Zeichen als Fehler.
3. Führe `mise run test:report-WRITES -- --apply` aus. CMake registriert die Datei beim Konfigurieren. Die Tests vergleichen alle Tabellen mit den Referenzschlüsseln. Nach einem neuen Windows-Build und Paketbau wählt `Language=fr` die Übersetzung. Die automatische Erkennung gilt weiterhin für Deutsch mit englischem Rückfall.

<a id="coverage"></a>

## Welche Mengen wurden im Spiel geprüft?

Die Mengenwahl bietet 1, 5, 10, Maximum und freie Schritte bis zur Materialgrenze. Bis einschließlich 0.0.65 sind alle 98 von 98 Profilen an allen neun Stationen durch Aufträge im Spiel belegt: alle drei am Herd, alle 23 am Alchemietisch (mit beiden Geisttrank-Zweigen), alle neun an der Lederbank, alle fünf an der Säge, beide an der Esse, alle 13 am Amboss, alle 20 an der Bogenbank, das Profil an der Kühlwanne sowie alle 22 am Schleifstein.

| Station (Profile) | Rezeptfamilien | Im Spiel hergestellte Beispiele |
| --- | --- | --- |
| Herd (3) | Fleisch, Molerat- und Pilzgericht | Fleisch ×7, Moleratgericht ×2, Pilzragout ×2 |
| Alchemietisch (23) | Heil- und Manatränke, Zusatzrezepte einschließlich Stängeln, Geisttrank, veredelter Tabak | Heilessenz ×2, Heilextrakt ×2, Heilelixier ×7, Manaessenz/-extrakt/-elixier ×2, zusammengesetzter Heil-/Manatrank ×2, Lous/Schlaf-/Fischschnaps ×2, permanente Gesundheit/Mana/Geschicklichkeit/Stärke ×2, Geschwindigkeitstrank ×2, Dracheneiertrank ×2, einfacher Stängel ×1/2/MAX 15, Grüner Novize ×2, angereicherter Geist ×1 und gewöhnlicher Geist ×2, Honig-/Sumpf-/Pilztabak ×1/2/MAX 3 |
| Lederbank (9) | Leder aus Fellen und Häuten | Bei zwei Fellen oder Häuten pro Auftrag ergaben Schaf, Lurker, Keiler und Warg je 2 Leder, Sumpfhai 4, Schattenläufer 6 und Troll 8. Frühere Aufträge ergaben 5 Leder aus 5 Wolfsfellen und 12 aus 2 schwarzen Trollfellen. |
| Säge (5) | Pfeile und Bolzen, auch Sondermunition | Je 2 Lose gewöhnliche Pfeile (60), Feuerpfeile (20), Magiepfeile (20), Bolzen (60) und Magiebolzen (20) |
| Esse (2) | Rohstahl erhitzen, heiße Klingen formen | Rohstahl ×2/5, heiße Klingen ×6 |
| Amboss (13) | Normal- und Erzwaffen | Gewöhnliches Schwert ×1/2; edles Schwert, edles Langschwert, Rubinklinge, El Bastardo und alle acht Erzwaffen (Erz-Langschwert bis großer Erz-Drachentöter) jeweils ×2 |
| Bogenbank (20) | Bögen und Armbrüste verbessern | Alle 20 Profile: Weidenbogen, Kompositbogen, Eschenbogen, Langbogen und leichte Armbrust ×1; Kurzbogen, guter Bogen, Jagdbogen, Ulmenbogen, Buchenbogen, Knochenbogen, Eichenbogen, Kriegsbogen, Drachenbogen, Armbrust, gute Armbrust, Kriegsarmbrust, schwere Armbrust und Drachenjägerarmbrust ×2; Jagdarmbrust bei MAX 2. |
| Kühlwanne (1) | Heiße Klingen abkühlen | ×2/1/MAX 3, auch die letzten zwei Lagerklingen gemeinsam |
| Schleifstein (22) | Waffen schleifen | Alle 22 Profile: grobes Schwert ×1/2/5/MAX 3; edles Schwert, Rubinklinge und Paladin-Zweihänder ×1; Paladinschwert, alle sechs gesegneten Erzklingen (1H/2H roh, gesegnet/Ordensschwert, Innos Zorn, Heiliger Vollstrecker), grober Zweihänder, grobes Bastardschwert, Zweihänder sowie alle acht Drachenjäger-Erzwaffen (Erz-Langschwert bis großer Erz-Drachentöter) jeweils ×2. |

Zusätzlich bietet das Lagerfeuer für gewöhnliches Fleisch den Originaldialog mit 1/5/10/alle. ENDE vor der Auswahl erhält den Bestand. Dieser Originaldialog steht zusätzlich zu den neun Stationen mit Mengenmenü zur Verfügung.

**Offene Profile.** Keine. Alle 98 integrierten Profile an allen neun Stationen sind durch Aufträge im Spiel belegt.

**Außerhalb der Mengenwahl.** Eine Rune je Zauber genügt. Runenverbesserungen kosten einmalig Lernpunkte und ändern den Lernzustand. Rüstungstausch und Wassermagierroben betreffen Einzelstücke, das Auge Innos ist ein einmaliger Questgegenstand. Vier Schärfversuche bleiben zurückgestellt, da jeder Versuch seinen eigenen Zufall und Fertigkeitsfortschritt hat.

<a id="tobacco"></a>

## Veredelten Tabak herstellen

Am Alchemietisch kannst du Honigtabak, Sumpftabak oder Pilztabak wählen. Jeder Durchgang verbraucht einen Apfeltabak und eine Laborflasche. Ein Honig, ein Sumpfkraut oder ein Pilz genügt als Zusatz für die ganze Menge. Beim Verlassen der Modauswahl vor „Herstellen“ bleibt der Bestand erhalten. Mehrere Stücke werden nur angeboten, wenn die zusätzlichen Apfeltabake und Flaschen aus den Lagerstapeln sicher entnommen werden können. Die drei Profile bestanden den Vergleich in der Spielwelt mit 0.0.52. Im Original kostet ZURÜCK nach dem Öffnen der Sortenwahl einen Apfeltabak.

<a id="cooling"></a>

## Zwei heiße Klingen an der Kühlwanne

Forme die beiden Klingen an der Esse. Öffne die Kühlwanne, wähle beim
Klingenrezept Menge 2 und bestätige „Herstellen“. Das ist auch
bei genau zwei heißen Klingen im Lager möglich. Die heiße Klinge in der Hand
gehört zur Stationsanimation. In der Weltabnahme sank der Lagerbestand von
zwei heißen auf null, der kalte Bestand stieg von sechs auf acht.
Nach dem regulären Stationsende erschien eine Mengenquittung.

Bei abweichender Bilanz sperrt die Mod den Folgeauftrag. Fällt die
Animationssteuerung (AniCtrl) erst nach dem Vorentzug aus, kann eine heiße
Lagerklinge verbraucht bleiben. Getestet mit Union/Shw32
(`78d9312f…742888`) und zBugFixes (`50c3dc66…11963`).
Neue Dateistände werden im Log vermerkt.

Die Auslieferung 0.0.43 ist außerdem mit Kühlung 2/1/Maximum 3 und mit
groben Schwertern in den Mengen 1/2/Schnellmenge 5/Maximum 3 weltgeprüft.
Bei einer abgelehnten Schleifprüfung nennt das Protokoll die
Prüfstelle und den erwarteten und beobachteten Wert.

<a id="recipes"></a>

## Rezepte und Mengen

Nach einem vollständig geprüften Erfolg erscheint die erzeugte Menge zusammen
mit den neuen Lagerbeständen. Beim nächsten Öffnen derselben Station bleibt
das Rezept gewählt; die Menge beginnt wieder bei eins. Bei Munition bezeichnet
die Menge Lose: drei gewöhnliche Lose entsprechen 90 Pfeilen bzw. Bolzen.
Die originale Arbeitskette und ihre Lernvoraussetzungen bleiben bestehen.
Der Katalog umfasst 13 zusätzliche Alchemierezepte, fünf gewöhnliche Ambosswaffen und
acht Erzwaffen sowie der Geisttrank mit seinen beiden originalen Ausgabevarianten.
Der gelernte Rezeptzweig bestimmt die Ausgabe; ein Wechsel verwirft den offenen
Auftrag. Beide Trankbestände werden anschließend kontrolliert. Ein Rezept wird
nur freigegeben, wenn der aktuelle Spielzustand
seinen geprüften Material- und Stationsbedingungen entspricht. Zusatzalchemie
rechnet auch die reservierte Laborflasche und die übrigen benötigten Flaschen
vollständig ab. Gemischt gelisteter und gepackter heißer Stahl bleibt vorerst
gesperrt; eine passende Originalauswahl bleibt erreichbar.

An der Bogenbank sind 20 Verbesserungen angebunden. „Teile für“ bezeichnet
die rechnerische Materialgrenze. Die gewählte Menge wird vor der Bestätigung
zusätzlich auf ihren tatsächlichen Inventarpfad geprüft. Ein Mengenwechsel
verlangt eine neue Prüfung; bei fehlendem Beleg bleibt „Herstellen“ gesperrt.
Ausgerüstete Ausgangswaffen bleiben gesperrt, bis sie abgelegt werden.
Falls das Original unterschiedliche Mindestbestände und Verbrauchsmengen
verlangt, zeigt die Oberfläche beide Werte getrennt. Beispielsweise benötigen
zwei Drachenjägerarmbrüste mindestens 24 Rohstahl im Vorrat, verbrauchen aber
20 Rohstahl und 24 Leder. Diese originale Besonderheit bleibt erhalten.

<a id="evidence"></a>

## Prüfstand und Grenzen

Im Spiel wurden alle 98 Profile aus der [Stationstabelle](#coverage) geprüft. Am Alchemietisch bestätigten die Tests die deutschen Rezepttexte und die angezeigten Sperrgründe. Zwei Portionen Sumpftabak verbrauchten zwei Apfeltabake und zwei Laborflaschen. Die physische Controllerbedienung an Stationen steht noch aus.

[Spielnachweise](../TESTED-WITH.md) · [Änderungen je Version](../CHANGELOG.md#deutsch) · [Projektübersicht](../README.de.md)
