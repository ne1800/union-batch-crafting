<a id="guide"></a>

# Union Batch Crafting

[English](STATIONS.md) · [Deutsch](STATIONEN.md)

In Marcello L’Hiver DE 2.0.4, the crafting menu supports 98 recipe profiles at the stove, alchemy table, leather bench, saw, forge, anvil, bow bench, cooling tub and grindstone. The menu shows learned recipes, materials, stock and a preview for the selected quantity.

<a id="install"></a>

## Player installation

Installing the player ZIP enables the plugin. To turn it off, set `Enabled=0` in `system/Gothic.INI`. [Install and uninstall the player ZIP](../INSTALL.md)

<a id="compatibility"></a>

## Tested with

The world comparisons used Gothic II: Night of the Raven, Marcello L’Hiver DE 2.0.4 and the following loaded files. The SHA-256 values identify the tested versions. After a module update, the earlier world results still apply to their original combination.

| Module | SHA-256 |
| --- | --- |
| Union 1.0m · Shw32.dll | `78d9312f07ebdf978d301fde63841ddcbe265b59cb7c2753e01d2dbdc5742888` |
| zBugFixes.dll | `50c3dc669a451dc084c4691b6e6fb7a39838d40627629e59e884579873811963` |
| New Vob Locator · UNION_VOB_LOCATOR.DLL | `72fc040ffe3d9ee6995493d4e5879336bbccf2f2301f5daa4578ff8f31b234fc` |
| Screen Info · UNION_GOTHIC_SCREEN_INFO.DLL | `c311eb0e3ef2416f4ba6f9c7beed7d1e01692622cdaa97e0ee0120ee086fc236` |

`UNION_GOTHIC_SCREEN_INFO.DLL` patches `zCMenu::ScreenInit`. The plugin accepts the tested variant when opening the quantity menu and the crafting receipt.

<a id="controls"></a>

## Controls

Open a supported station. Use Up/Down to select a known recipe, Tab to change focus, and Left/Right to adjust the quantity by one. Use −5/+5 or the 1/5/10/maximum presets for larger batches. Review the materials and preview, then choose Craft and confirm with A/Enter. B/Escape goes back. Choose the original controls to return to the game's selection. The menu uses the existing controller mapping. Physical controller input at a station still needs testing.

The `>` arrow marks the active control. When you edit an amount, `[x]` marks the selected recipe in the list. Page Up adds five and Page Down subtracts five. An ingredient marked as held counts toward the amount used.

Blocked recipes show the reason in brackets, such as “[missing ingredients]” or “[no space]”. The craft action shows the reason for the selected amount when known. While its availability is being checked, it shows “[locked]”.

<a id="configuration"></a>

## Configuration and language selection

This optional section in `system/Gothic.INI` sets the language for the plugin menus:

```ini
[UNIONBATCHCRAFTING]
Enabled=1
Language=auto
```

With `auto`, a blank value or a missing entry, the plugin detects German game text and falls back to English. Use `de` to force German or `en` to force English. Unknown values select English. The game controls the language of its original dialogues.

<a id="adding-a-language"></a>

## Adding a language

1. In the source repository, copy `include/ubc/text/locales/en.hpp` to a sibling file such as `fr.hpp`. Use a lowercase two-letter code and change the namespace in that copy to `locale_fr`.
2. Translate the text in `messages`, `items` and `recipes`. Preserve every key, the numerically sorted IDs, and the order and types of printf arguments. Save the source as UTF-8 and write the actual characters in the Unicode literals. Set `codepage` to the Windows codepage used by the game (1252 for German and English, 1250 for Polish, 1251 for Russian). The build converts each character to one game byte and rejects unrepresentable characters.
3. Run `mise run test:report-WRITES -- --apply`. CMake registers the file at configure time. The tests compare every table against the reference keys. After rebuilding for Windows and packaging the DLL, select the translation with `Language=fr`. Automatic detection still selects German with English fallback.

<a id="coverage"></a>

## Which quantities have been tested in the game?

The quantity menu offers 1, 5, 10, maximum and custom steps up to the material limit. In-game orders through 0.0.64 cover 70 of the 98 profiles, including all 23 at the alchemy table (with both spirit potion branches), all 13 at the anvil, all nine at the leather bench, all five at the saw, 11 of the 20 at the bow bench and four of the 22 at the grindstone.

| Station (profiles) | Recipe families | Examples crafted in the game |
| --- | --- | --- |
| Stove (3) | Meat, molerat dish and mushroom stew | Meat ×7, mushroom stew ×2 |
| Alchemy table (23) | Healing and mana potions, additional recipes including swampweed rolls, spirit potion, refined tobacco | Healing essence ×2, healing extract ×2, healing elixir ×7, mana essence/extract/elixir ×2, compound healing/mana potion ×2, Lou’s/sleep/fish booze ×2, permanent health/mana/dexterity/strength ×2, speed potion ×2, dragon egg potion ×2, simple roll ×1/2/MAX 15, Green Novice ×2, enriched spirit ×1 and ordinary spirit ×2, honey/swampweed/mushroom tobacco ×1/2/MAX 3 |
| Leather bench (9) | Leather from hides and skins | With two hides per order, sheep, lurker, boar and warg each yielded 2 leather, swampshark 4, shadowbeast 6 and troll 8. Earlier orders yielded 5 leather from 5 wolf skins and 12 from 2 black troll hides. |
| Saw (5) | Arrows and bolts, including special ammunition | 2 lots each of ordinary arrows (60), fire arrows (20), magic arrows (20), bolts (60) and magic bolts (20) |
| Forge (2) | Heat raw steel, form hot blades | Raw steel ×2/5, hot blades ×6 |
| Anvil (13) | Ordinary and ore weapons | Ordinary sword ×1/2; fine sword, fine longsword, ruby blade, El Bastardo and all eight ore weapons (ore longsword through great ore dragon slayer) ×2 each |
| Bow bench (20) | Bow and crossbow upgrades | One each of the willow bow, composite bow, ash bow, longbow and light crossbow. Two each of the shortbow, fine bow, hunting bow, crossbow and fine crossbow. Hunting crossbow at MAX 2. |
| Cooling tub (1) | Cool hot blades | ×2/1/MAX 3, including the last two stored blades together |
| Grindstone (22) | Grind weapons | Rough sword ×1/2/5/MAX 3. Two each of the rough two-hander, rough bastard sword and two-hander. |

The campfire also offers the original 1/5/10/all choices for ordinary meat. Choosing END before selecting a quantity preserves stock. This dialogue is separate from the nine stations with a quantity menu.

**Untested profiles.** The 28 profiles awaiting an in-game order are 18 grindstone weapons, nine bow-bench upgrades and one stove meal (molerat dish). Of the nine remaining bow-bench upgrades, six lacked the base weapon in the unmodified save and three needed more than the eight leather on hand; the oak bow also failed its material check in an earlier run.

**Outside the quantity menu.** One rune per spell suffices. Rune upgrades spend learning points once and change learned abilities. Armour exchanges and water mage robes concern individual equipment pieces, and the Eye of Innos is a one-off quest item. Four sharpening attempts remain deferred because each rolls its own outcome and advances proficiency individually.

<a id="tobacco"></a>

## Refined tobacco

Choose honey, swampweed or mushroom tobacco at the alchemy table. Each portion costs one apple tobacco and one laboratory flask. One honey, swampweed or mushroom is sufficient for the entire order. Cancelling the mod menu before Craft preserves stock. Multiple portions require safe removal of the extra apple tobacco and flasks from storage. The three profiles passed the in-game comparison in 0.0.52. In the original dialogue, choosing BACK after opening the variety selector consumes one apple tobacco.

<a id="cooling"></a>

## Cooling two hot blades

Form both blades at the forge. At the cooling tub, select the blade recipe, choose quantity 2 and confirm Craft. This works with exactly two hot blades in storage. The hot blade in the hand belongs to the station animation. During acceptance the hot stock fell from two to zero and the cold stock rose from six to eight. A quantity receipt appeared after normal station exit.

A stock mismatch blocks the next order. If animation control (AniCtrl) fails after the initial debit, one stored hot blade can remain consumed. The tested modules were Union/Shw32 (`78d9312f…742888`) and zBugFixes (`50c3dc66…11963`). The plugin logs new module versions.

The 0.0.43 build also passed cooling at 2/1/maximum 3 and rough swords at 1/2/preset 5/maximum 3. If a grinding check fails, the log shows the check name and its expected and observed values.

<a id="recipes"></a>

## Recipes and quantities

After a verified order the receipt shows the output and updated stocks. Reopening the same station retains the recipe and resets quantity to one. Ammunition quantities count lots. Three ordinary lots make 90 arrows or bolts. Original work sequences and learning requirements apply.

The catalogue includes 13 additional alchemy recipes, five ordinary anvil weapons, eight ore weapons and both original spirit potion outputs. The learned branch determines the output. Changing branches discards an open order. Both potion stocks are checked afterwards. Crafting requires a match to the recipe's verified material and station conditions. Additional alchemy accounts for the reserved laboratory flask and every other required flask. Hot steel split between listed and packed stacks remains blocked. The corresponding original selection is available.

The bow bench supports 20 upgrades. “parts for” is the material limit. The menu checks the actual inventory path for the selected quantity before confirmation. Changing the quantity requires a new check. Craft stays locked until it passes. The UI distinguishes minimum stock from consumption when the original requires different values. Two dragon hunter crossbows need at least 24 raw steel on hand but consume 20 steel and 24 leather.

<a id="evidence"></a>

## Test coverage and limits

In-game tests covered the orders in the [station table](#coverage). At the alchemy table, the tests confirmed the German recipe text and the reasons shown for blocked recipes. Crafting two portions of swamp tobacco consumed two apple tobaccos and two laboratory flasks. Some recipes and physical controller input at stations still need testing.

[In-game test details](../TESTED-WITH.md) · [Changes by version](../CHANGELOG.md#english) · [Project overview](../README.md)
