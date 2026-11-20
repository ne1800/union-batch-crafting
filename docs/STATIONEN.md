<a id="guide"></a>

# Union Batch Crafting 0.0.57

[English](STATIONS.md) · [Deutsch](STATIONEN.md)

Die lokale Testfassung für Marcello L’Hiver DE 2.0.4 verbindet 98 Rezeptprofile an Herd, Alchemietisch, Lederbank, Säge, Esse, Amboss, Bogenbank, Kühlbecken und Schleifstein. Das Menü zeigt gelernte Rezepte, Bedarf, Vorrat und die Vorschau für die gewählte Menge.

<a id="install"></a>

## Installation für Spieler

Das Spieler-ZIP **0.0.57** bleibt nach der Installation aktiv. In Gothic.INI schaltet `Enabled=0` es ab. [Spielerinstallation und Deinstallation](../INSTALL.de.md)

<a id="compatibility"></a>

## Getestet mit

Die Weltvergleiche gelten für Gothic II DNdR, Marcello L’Hiver DE 2.0.4 und die folgenden geladenen Dateien. Die SHA-256-Werte identifizieren die getesteten Stände. Nach einem Modulupdate gelten die früheren Weltbelege weiterhin für ihre ursprüngliche Kombination.

| Modul | SHA-256 |
| --- | --- |
| Union 1.0m · Shw32.dll | `78d9312f07ebdf978d301fde63841ddcbe265b59cb7c2753e01d2dbdc5742888` |
| zBugFixes.dll | `50c3dc669a451dc084c4691b6e6fb7a39838d40627629e59e884579873811963` |
| New Vob Locator · UNION_VOB_LOCATOR.DLL | `72fc040ffe3d9ee6995493d4e5879336bbccf2f2301f5daa4578ff8f31b234fc` |

<a id="controls"></a>

## Bedienung

Öffne eine angebundene Station. Wähle mit Auf/Ab ein bekanntes Rezept. Mit Tab wechselst du den Bereich, mit Links/Rechts die Menge um eins. Für größere Mengen gibt es −5/+5 sowie 1/5/10/Maximum. Prüfe Bedarf und Vorschau, wähle „Herstellen“ und bestätige mit A/Enter. B/Escape geht zurück. Über „Originale Bedienung“ erreichst du die ursprüngliche Auswahl. Die vorhandene Controllerbelegung bleibt nutzbar. Die physische Bedienung an Stationen ist noch offen.

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
2. Übersetze die Texte in `messages`, `items` und `recipes`. Behalte jeden Schlüssel, die numerisch sortierten IDs und die Reihenfolge und Typen der printf-Platzhalter bei. Verwende für die Gothic-Anzeige geeignete Zeichen. Die vorhandenen Tabellen verwenden ASCII.
3. Führe `mise run test:report-WRITES -- --apply` aus. CMake registriert die Datei beim Konfigurieren. Die Tests vergleichen alle Tabellen mit den Referenzschlüsseln. Nach einem neuen Windows-Build und Paketbau wählt `Language=fr` die Übersetzung. Die automatische Erkennung gilt weiterhin für Deutsch mit englischem Rückfall.

<a id="coverage"></a>

## Welche Mengen wurden im Spiel geprüft?

Die Mengenwahl bietet 1, 5, 10, Maximum und freie Schritte bis zur Materialgrenze. Die Tabelle unterscheidet den Katalogstand 0.0.54 von den tatsächlich ausgeführten Aufträgen früherer Fassungen.

| Station (Profile) | Rezeptfamilien | Im Spiel hergestellte Beispiele |
| --- | --- | --- |
| Herd (3) | Fleisch, Molerat- und Pilzgericht | Fleisch ×7, Pilzragout ×2 |
| Alchemietisch (23) | Heil- und Manatränke, Zusatzrezepte einschließlich Stängeln, Geisttrank, veredelter Tabak | Heilessenz ×2, Heilextrakt ×2, Heilelixier ×7, zusammengesetzter Heiltrank ×2, einfacher Stängel ×1/2/MAX 15, Grüner Novize ×2, angereicherter Geist ×1. Honig-/Sumpf-/Pilztabak ×1/2/MAX 3 |
| Lederbank (9) | Leder aus Fellen und Häuten | 5 Wolfsfelle ergaben 5 Leder, 2 schwarze Trollfelle ergaben 12 Leder |
| Säge (5) | Pfeile und Bolzen, auch Sondermunition | 2 Lose gewöhnliche Pfeile ergaben 60 Pfeile |
| Esse (2) | Rohstahl erhitzen, heiße Klingen formen | Rohstahl ×2/5, heiße Klingen ×6 |
| Amboss (13) | Normal- und Erzwaffen | Gewöhnliches Schwert ×1/2 |
| Bogenbank (20) | Bögen und Armbrüste verbessern | Weidenbogen ×1, Kurzbogen ×2, leichte Armbrust ×1, Jagdarmbrust MAX 2 |
| Kühlwanne (1) | Heiße Klingen abkühlen | ×2/1/MAX 3, auch die letzten zwei Lagerklingen gemeinsam |
| Schleifstein (22) | Waffen schleifen | Grobes Schwert ×1/2/5/MAX 3 |

Zusätzlich bietet das Lagerfeuer für gewöhnliches Fleisch den Originaldialog mit 1/5/10/alle. ENDE vor der Auswahl erhält den Bestand. Dieser Originaldialog steht zusätzlich zu den neun Stationen mit Mengenmenü zur Verfügung.

**Offene Profile.** Die Weltläufe deckten einzelne gelernte Rezepte und verfügbare Vorräte ab. Für die übrigen Lederarten, Waffen, Munition, Gerichte und Tränke fehlt ein entsprechender Auftrag. Beim Geisttrank war der angereicherte Zweig aktiv und nur ein Pfeffer vorhanden. An der Bogenbank verbrauchten die vier Testaufträge alle acht Leder. Je nach Profil fehlten Leder, Stahl oder die Ausgangswaffe. Beim Eichenbogen scheiterte zusätzlich die Materialprüfung. Für 21 Schleifprofile steht der eigene Weltvergleich aus.

**Außerhalb der Mengenwahl.** Eine Rune je Zauber genügt. Runenverbesserungen kosten einmalig Lernpunkte und ändern den Lernzustand. Rüstungstausch und Wassermagierroben betreffen Einzelstücke, das Auge Innos ist ein einmaliger Questgegenstand. Vier Schärfversuche bleiben zurückgestellt, da jeder Versuch seinen eigenen Zufall und Fertigkeitsfortschritt hat.

<a id="tobacco"></a>

## Veredelten Tabak herstellen

Am Alchemietisch kannst du Honigtabak, Sumpftabak oder Pilztabak wählen. Jeder Durchgang verbraucht einen Apfeltabak und eine Laborflasche. Ein Honig, ein Sumpfkraut oder ein Pilz genügt als Zusatz für die ganze Menge. Beim Verlassen der Modauswahl vor „Herstellen“ bleibt der Bestand erhalten. Mehrere Stücke werden nur angeboten, wenn die zusätzlichen Apfeltabake und Flaschen aus den Lagerstapeln sicher entnommen werden können. Die drei Profile bestanden den Vergleich in der Spielwelt mit 0.0.52. Im Original kostet ZURÜCK nach dem Öffnen der Sortenwahl einen Apfeltabak.

<a id="cooling"></a>

## Zwei heiße Klingen an der Kühlwanne

Forme die beiden Klingen an der Esse. Öffne die Kühlwanne, wähle beim
Klingenrezept Menge 2 und bestätige „Herstellen“. Seit 0.0.43 ist das auch
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
In 0.0.44 nennt das Protokoll bei einer abgelehnten Schleifprüfung die
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

An der Bogenbank sind 20 Verbesserungen angebunden. „Vorrat für“ bezeichnet
die rechnerische Materialgrenze. Die gewählte Menge wird vor der Bestätigung
zusätzlich auf ihren tatsächlichen Inventarpfad geprüft. Ein Mengenwechsel
verlangt eine neue Prüfung; bei fehlendem Beleg bleibt „Herstellen“ gesperrt.
Falls das Original unterschiedliche Mindestbestände und Verbrauchsmengen
verlangt, zeigt die Oberfläche beide Werte getrennt. Beispielsweise benötigen
zwei Drachenjägerarmbrüste mindestens 24 Rohstahl im Vorrat, verbrauchen aber
20 Rohstahl und 24 Leder. Diese originale Besonderheit bleibt erhalten.

<a id="evidence"></a>

## Prüfstand und Grenzen

0.0.55 dokumentiert den Funktionsstand von 0.0.54 und wurde offline geprüft. Der letzte Weltbeleg von 0.0.53 bestätigt die automatische deutsche Mengenanzeige und den kostenfreien Abbruch. Die Stationsvergleiche oben stammen aus früheren Läufen. Für die physische Controllerbedienung an Stationen steht die Prüfung aus. Über die Originalbedienung bleiben die ursprünglichen Rezepte erreichbar. [Projektübersicht](../README.de.md) · [Änderungen je Version](../CHANGELOG.md#deutsch)
