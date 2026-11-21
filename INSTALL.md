<a id="player"></a>

# Union Batch Crafting 0.0.60 for players

[English](INSTALL.md) · [Deutsch](INSTALL.de.md) · [Tested with](TESTED-WITH.md)

<a id="install"></a>

## Install

1. Use Gothic II: Night of the Raven with Union already installed and Marcello L’Hiver DE selected. Close the game and the Workshop launcher. In Steam, open the game's **Properties → Installed Files → Browse**. The right folder contains `system/Gothic2.exe`.
2. Open `union-batch-crafting-player-0.0.60.zip`. Copy its `system/autorun/UnionBatchCraftingExtended.dll` into the matching folder in the game. Use the existing spelling of the autorun folder on Linux/Proton. The final path, relative to the game folder, is `system/autorun/UnionBatchCraftingExtended.dll`. If that DLL already exists, move it outside the game first and keep it for rollback. Keep the guides and licences wherever you like.
3. Start Gothic as usual, select Marcello L’Hiver and load a save. At a supported station with a learned recipe, choose the recipe and quantity and confirm Craft. The DLL stays enabled on subsequent starts. Union loads it from the autorun directory.

For a first check, take two pieces of raw meat to a stove and select meat at quantity 2. A campfire uses the game’s own meat choices.

The player DLL from 0.0.57 loaded without a marker. At the alchemy table, selecting two swamp tobaccos used two apple tobaccos and two laboratory flasks. Swampweed was unchanged. In 0.0.59, the German quantity menu displayed “2 / 6 Durchgänge” and “Zurück”. The game used the expected Windows-1252 byte E4 for ä. Crafting two swamp tobaccos consumed two apple tobaccos and two laboratory flasks.

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

## Other versions

The plugin checks engine compatibility and the script functions and symbols needed for each recipe. A mismatch leaves the original crafting selection available. A shared engine-layout mismatch can disable the entire plugin. For a loaded plugin, `system/autorun/UnionBatchCraftingExtended.log` contains the current run's diagnostics. The autorun folder must be writable to create the log. Earlier in-game results and the scope of this build's tests are listed in [Tested with](TESTED-WITH.md).

<a id="uninstall"></a>

## Uninstall

Close Gothic and the launcher. Delete the installed `system/autorun/UnionBatchCraftingExtended.dll` and its `UnionBatchCraftingExtended.log` if present. If you backed up an earlier DLL and want it back, restore it now. Optionally remove the two settings and the now-empty `[UNIONBATCHCRAFTING]` section. Keep other INI sections, Union plugins and saves. Crafting changes inventory during play and persists if you save.
