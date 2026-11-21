<a id="overview"></a>

# Union Batch Crafting

[English](README.md) · [Deutsch](README.de.md)

German menus display umlauts and ß. The **0.0.61 player ZIP** stays enabled after installation. Set `Enabled=0` in Gothic.INI to switch it off. [Install and uninstall the player ZIP](INSTALL.md)

Blocked recipes show the reason in brackets, such as “[missing ingredients]” or “[no space]”. The craft action shows the reason for the selected amount when known. While its availability is being checked, it shows “[locked]”.

Wrapped lines start in the recipe-name column. A larger gap separates adjacent recipes.

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

Build the player ZIP with `mise run package:player-WRITES -- --apply`.

<a id="development"></a>

## Development

Run `mise run lint:all` to check all tracked files with prek. prek installs pinned versions of all formatters and linters on first use, including clang-format 23.1.5 and gersemi 0.29.2. Run `mise run test:report-WRITES -- --apply` for the portable tests. The private development checkout enables pre-commit and commit-msg with `mise x -- prek install`. In the source export, run the mise tasks directly.

Write Conventional Commit subjects in English, for example `fix(ui): handle empty recipe lists`. The scope is optional. Add `!` before the colon for a breaking change.

<a id="ci"></a>

## Continuous integration

Development takes place in the private GitLab repository. Push a feature branch, open a merge request with an English Conventional Commit title and merge into `main` after the pipeline passes. GitLab checks commit subjects, prek, portable tests, sanitizers, the Windows x86 build and a temporary source export with a leak scan.

GitHub checks the exported sources on pushes and pull requests. Both systems cache the pinned Windows SDK and compare the player and test DLLs with the SHA-256 values for 0.0.61. A matching version tag builds the player ZIP and creates a draft for manual review and publication. Game and controller tests run locally.

<a id="renovate"></a>

## Renovate

Renovate opens update notifications on GitHub. Actions checks each PR. A maintainer imports it into the private development branch, runs the tests and exports the accepted update. Leave the GitHub PR open for Renovate to close after the export arrives.

The update window opens on Mondays before 06:00 (Europe/Berlin). Updates mature for 14 days. Approve entries labelled `dll-impact` in the dashboard and run level 2 checks with both DLL hashes. Changed binaries may require an approved world test. For xwin, independently verify the archive and update its SHA-256 manually. Until then Windows CI fails the checksum check. Maintain the Arch archive date in `tools/ci/bootstrap.sh` and the Windows SDK manifest manually.
