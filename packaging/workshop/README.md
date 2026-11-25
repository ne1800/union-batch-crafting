# Publishing Union Batch Crafting on Steam Workshop

[Deutsch](README.de.md)

## Build the addon

From the source checkout with the Windows build tools configured, build the pinned player DLL and its Workshop archive. The last command reads the archive back, checks the SHA-256 against `tools/ci/dll.sha256` and checks both licence texts.

```sh
mise run build:windows -- --target UnionBatchCraftingPlayer
mise run package:workshop-WRITES -- --apply
mise run package:verify-workshop
```

The content directory contains one file, `Data/UnionBatchCrafting.vdf`. Its virtual `SYSTEM/AUTORUN` directory holds the player DLL. `DOCS/UNIONBATCHCRAFTING` holds `LICENSE` and `THIRD-PARTY-NOTICES` so subscribers receive the notices together with the binary and the texts stay outside the autorun directory. The packer fixes the catalogue order and uses a DOS timestamp of 2000-01-01. The launcher adjusts the header timestamp of its installed copy for load order.

To repeat the format comparison against the installed ScreenInfo plugin (Workshop item 2787243474), set `UBC_SCREENINFO_VDF` to the full path of its VDF and run the package tests from the checkout.

```sh
mise x -- python3 -B -m unittest discover -s tests/python/package -p test_workshop.py
```

## Prepare Steam and the preview

Keep the Steam desktop client running and signed in to the account that will own the item. Accept the Steam Subscriber Agreement and any Workshop terms requested for that account. Use the Gothic II Workshop installation with the uploader and `steam_appid.txt` in its `launcher` directory. Test the physical controller in game. Approve the 500 × 500 JPG or PNG and check the rights to the depicted game and mod content before publication. Keep the preview outside `content`.

Set these variables in **Bash** at the source checkout. Replace the two example paths and keep this shell open for the chosen operation. Wine's standard `Z:` drive maps to the Linux filesystem. The working directory below puts the uploader beside its Steam App ID file.

```bash
repo=$(pwd -P)
game='/path/to/SteamLibrary/steamapps/common/Gothic II'
art='/path/to/approved-preview.jpg'
meta="$repo/packaging/workshop"
winpath() { local p; p=$(realpath -e -- "$1") || return; printf 'Z:%s\n' "${p//\//\\}"; }
content=$(winpath "$repo/dist/workshop/content") && preview=$(winpath "$art") || exit 1
cd -- "$game/launcher" || exit 1
```

## Publish a new item

Run once when ready to create the hidden item. This sends the English title and Steam BBCode description as separate arguments.

```bash
protontricks-launch --appid 39510 ./GothicSteamWorkshopUploader.exe publish \
  --game=g2 --title="$(cat "$meta/title.en.txt")" \
  --description="$(cat "$meta/description.en.bbcode")" --tags="$(cat "$meta/tags.txt")" \
  --content="$content" --preview="$preview"
```

On the hidden Workshop page, add only Union (2787458679) under **Required Items**. The Gothic launcher requires all listed items. Steamworks provides no optional flag for this relationship. Link the optional Marcello L’Hiver DE campaign (2847787799) in the description. [Steamworks](https://partner.steamgames.com/doc/api/ISteamUGC#AddDependency) · [Gothic Workshop Guide](https://steamcommunity.com/sharedfiles/filedetails/?id=2779846714)

Add the German title and description from `title.de.txt` and `description.de.bbcode`. Subscribe to the hidden item and enable it with Union. Test German Night of the Raven 2.6 without a campaign mod and Marcello L’Hiver DE 2.0.4 in game before making the page public. When moving from a manual installation, close Gothic and remove the loose `system/autorun/UnionBatchCraftingExtended.dll` first.

## Update the existing item

After rebuilding and checking the updated archive, set the existing Workshop item ID and a changenote describing the actual change. The content-only example retains the existing page artwork. Pass `--preview="$preview"` as an additional argument when replacing it with a newly approved image.

```bash
item_id='<ID>'
change='<changenote>'
protontricks-launch --appid 39510 ./GothicSteamWorkshopUploader.exe update \
  --game=g2 --mod="$item_id" --changenote="$change" --content="$content"
```

## Offline references

The installed `launcher/GothicSteamWorkshopGuide.pdf` describes the required addon tag, directory structure, preview dimensions, hidden visibility, dependencies and both uploader commands. Its Workshop rules require a finished and tested, non-commercial submission respecting third-party rights. The installed `launcher/example_mod/publish.bat` demonstrates `--game=g2` and the separate content and preview paths. Local `protontricks-launch --help` documents the `--appid APPID executable [args]` syntax. The example commands were checked against these sources offline. Publication and updates require the owner's live Steam session.
