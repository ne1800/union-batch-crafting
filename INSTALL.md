<a id="player"></a>

# Union Batch Crafting 0.0.68 for players

[English](INSTALL.md) · [Deutsch](INSTALL.de.md) · [Tested with](TESTED-WITH.md)

<a id="install"></a>

## Install

1. Install Union for Gothic II: Night of the Raven. Close the game and the Workshop launcher. In Steam, open the game's **Properties → Installed Files → Browse**. The right folder contains `system/Gothic2.exe`.
2. Open `union-batch-crafting-player-0.0.68.zip`. Copy its `system/autorun/UnionBatchCraftingExtended.dll` into the matching folder in the game. Use the existing spelling of the autorun folder on Linux/Proton. The final path, relative to the game folder, is `system/autorun/UnionBatchCraftingExtended.dll`. If that DLL already exists, move it outside the game first and keep it for rollback. Keep the guides and licences wherever you like.
3. Start Gothic as usual, select the original game or Marcello L’Hiver DE 2.0.4, and start or load a game. At a supported station with a learned recipe, choose the recipe and quantity and confirm Craft. The DLL stays enabled on subsequent starts. Union loads it from the autorun directory.

For a first check, take two pieces of raw meat to a stove and select quantity 2.

<a id="configure"></a>

## Configure

Close Gothic. To override the defaults, edit `system/Gothic.INI` beside `Gothic2.exe`. Add this section once or edit its existing keys. Changes apply at the next start.

```ini
[UNIONBATCHCRAFTING]
Enabled=1
Language=auto
```

`Enabled=0` turns the plugin off. Removing the key or setting it to 1 enables it. For the quantity menu, `auto` detects German game text and otherwise uses English. Choose `de` or `en` to force a language.

<a id="compatibility"></a>

## Supported editions and stations

The supported unmodded edition is German 2.6, using the loose `_work/Data/Scripts/_compiled/GOTHIC.DAT` shipped with Gothic II: Gold Classic on Steam (Workshop branch, build 9820952). The plugin stays idle with an unrecognized DAT, including other language and patch versions.

- **Night of the Raven without a campaign mod** – alchemy table (20 potion and specialty recipes, three tobacco blends), anvil (five regular and eight ore weapons), forge, frying pan and stove.
- **Marcello L’Hiver DE 2.0.4** – 98 recipe profiles across the stove, alchemy table, leather bench, saw, forge, anvil, bow bench, cooling tub and grindstone.

In Night of the Raven, the forge, frying pan and stove offer 1, a custom quantity or the maximum. Each piece plays its original animation. Cancelling in the quantity menu costs nothing. As in the original game, pressing Marvin F8 after reservation can cost one cold raw steel at the forge or one raw meat at the stove. A hard interruption of the stove animation can also consume the reserved meat.

The plugin checks engine compatibility and the script functions and symbols needed for each recipe. A mismatch leaves the original crafting selection available. A shared engine-layout mismatch can disable the entire plugin. For a loaded plugin, `system/autorun/UnionBatchCraftingExtended.log` contains the current run's diagnostics. The autorun folder must be writable to create the log. Earlier in-game results and the scope of this build's tests are listed in [Tested with](TESTED-WITH.md).

<a id="uninstall"></a>

## Uninstall

Close Gothic and the launcher. Delete the installed `system/autorun/UnionBatchCraftingExtended.dll` and its `UnionBatchCraftingExtended.log` if present. If you backed up an earlier DLL and want it back, restore it now. Optionally remove the two settings and the now-empty `[UNIONBATCHCRAFTING]` section. Keep other INI sections, Union plugins and saves. Crafting changes inventory during play and persists if you save.
