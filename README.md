<a id="overview"></a>

# Union Batch Crafting

[English](README.md) · [Deutsch](README.de.md)

The **0.0.57 player ZIP** stays enabled after installation. Set `Enabled=0` in Gothic.INI to switch it off. [Install and uninstall the player ZIP](INSTALL.md)

Choose a recipe and craft several portions at once in Gothic II: Night of the Raven with Marcello L’Hiver DE 2.0.4 and Union. Version 0.0.57 covers 98 profiles across nine stations. The guide lists the quantities tested in the game and explains custom quantities, cancellation, and the limits of controller testing. [Station guide](docs/STATIONS.md) · [Changelog](CHANGELOG.md#english)

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

Build the player ZIP with `mise run package-player-WRITES -- --apply`.
