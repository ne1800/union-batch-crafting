<a id="overview"></a>

# Union Batch Crafting

[English](README.md) · [Deutsch](README.de.md)

A Union plugin for Gothic II: Night of the Raven with the Marcello L’Hiver DE 2.0.4 mod. At a crafting station, pick a learned recipe and a quantity, check the materials and the preview, and craft the whole batch at once instead of repeating the original dialogue for every piece.

<a id="features"></a>

## Features

- One quantity menu for 98 recipe profiles at nine stations: stove, alchemy table, leather bench, saw, forge, anvil, bow bench, cooling tub and grindstone.
- Presets for 1, 5, 10 and maximum, plus any custom amount up to the material limit.
- Required materials, current stock and a preview of the result for the selected amount.
- Blocked recipes name the reason, such as “[equipped]”, “[missing ingredients]” or “[no space]”.
- An order only runs when the materials and station conditions match the verified recipe. Otherwise the game’s original selection stays available.
- German and English menus, detected from the game text or set in `Gothic.INI`. Further languages can be added to the source as text tables.
- Keyboard controls and the game’s existing controller mapping.

[Station guide](docs/STATIONS.md) · [Controls](docs/STATIONS.md#controls) · [Add a language](docs/STATIONS.md#adding-a-language)

<a id="status"></a>

## Status

This is an early release. In-game tests covered selected recipes and amounts at every station, listed in the [station guide](docs/STATIONS.md#coverage). The other profiles still await an in-game order. Station controls have not yet been tested with a physical controller. The plugin checks the engine and the script functions each recipe needs. On a mismatch the original selection stays available, and an engine mismatch can disable the whole plugin. [Tested with](TESTED-WITH.md) · [Changelog](CHANGELOG.md#english)

<a id="install"></a>

## Installation

Download `union-batch-crafting-player-<version>.zip` from the [releases](https://github.com/ne1800/union-batch-crafting/releases) and copy `system/autorun/UnionBatchCraftingExtended.dll` into the game folder. The plugin stays enabled until you remove the DLL or set `Enabled=0` in `system/Gothic.INI`. [Installation, settings and removal](INSTALL.md)

<a id="build"></a>

## Build from source

On Linux, use Clang with C++17 support, CMake 3.21 or later, Ninja, Python 3.10 or later and mise. From the root of this checkout, run the portable build and tests:

```sh
mise trust
mise run build:native
ctest --test-dir build/native --output-on-failure
mise run test:tools
```

The Python suite checks both translations and their links. The native tests use synthetic fixtures and the recipe table. Positive checks against the original executable and mod archive require a matching local game installation and are skipped in a portable run.

To cross-compile the plugin, you also need clang-cl, LLD, llvm-rc, llvm-mt and the x86 Windows SDK/CRT. If you accept Microsoft's SDK licence, the optional setup command below downloads the pinned xwin tool and prepares the SDK in `.tools/xwin`. If it is already present, start with the second command.

```sh
mise run setup:windows-WRITES -- --apply
mise run build:windows
```

The plugin is `build/windows/player/UnionBatchCraftingExtended.dll`. Building it requires no game files. The project's own code uses the [MIT licence](LICENSE). xwin uses MIT/Apache-2.0. Microsoft's terms apply to the SDK and to the release CRT linked into the Windows DLL.

Build the player ZIP with `mise run package:player-WRITES -- --apply`.

<a id="development"></a>

## Contributing

Report bugs and suggest changes in GitHub issues. Send code changes as pull requests against `main`. A maintainer reviews each pull request.

Write commit subjects and pull request titles as English Conventional Commits, for example `fix(ui): handle empty recipe lists`. The scope is optional. Add `!` before the colon for a breaking change.

Run `mise run lint:all` to check all tracked files with prek. prek installs pinned versions of all formatters and linters on first use, including clang-format 23.1.5 and gersemi 0.29.2. Run `mise run test:report-WRITES -- --apply` for the portable tests. After `mise x -- prek install` the checks run before each commit.

The automated tests do not start the game. If your change affects crafting in the game, describe in the pull request what you tested in the game and with which setup. [Tested with](TESTED-WITH.md) lists the reference setup.

<a id="ci"></a>

## Continuous integration

GitHub Actions checks every pull request and every push to `main` with prek, the portable tests and a Windows x86 build. The build must reproduce the SHA-256 values of the player and test DLLs in `tools/ci/dll.sha256`. If your change alters a DLL, the job log shows the new values. Update the file in the same pull request, or run `mise run release:prepare-WRITES -- --pin-dlls --apply` after a Windows build.

The file `VERSION` names the current version. `mise run release:prepare-WRITES -- --version <x.y.z> --apply` sets it in the guides and adds changelog stubs in both languages. A pull request that changes `VERSION` must also record the player DLL from its in-game test in [Tested with](TESTED-WITH.md). When the maintainer merges it, GitHub Actions tags the version and publishes the release with the player ZIP, the changelog entry and the SHA-256 values.

<a id="renovate"></a>

## Dependency updates

Renovate proposes updates for pinned tools and GitHub Actions as pull requests, on Mondays before 06:00 (Europe/Berlin) and after a 14-day waiting period. Updates to the Windows build toolchain change the DLLs and wait for approval on the dependency dashboard.

<a id="license"></a>

## License

The project’s own code uses the [MIT licence](LICENSE). The [third-party notices](THIRD-PARTY-NOTICES) cover the Microsoft runtime linked into the DLL. The repository contains no game files, original scripts or saves.
