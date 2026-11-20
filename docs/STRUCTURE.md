<a id="structure"></a>

# Source layout

[English](STRUCTURE.md) · [Deutsch](STRUKTUR.md)

| Directory | Responsibility |
| --- | --- |
| `src/plugin/` | This directory contains the DLL entry point, resources and Windows integration for crafting. |
| `src/plugin/crafting/` | These translation units share station state and handle dialogue, quantity selection, inventory updates and completion. |
| `src/bridges/` | These adapters call engine functions for frames, quantities, ammunition and material consumption. |
| `src/writers/` | These routines apply checked inventory changes for alchemy, the spirit potion and tobacco. |
| `src/ui/` | These adapters connect menu models and language selection to the native interface. |
| `src/probes/` | Six separate diagnostic plugins provide entry points and export definitions for test hosts and diagnostic packages. |
| `include/ubc/catalogs/` | This directory holds recipe descriptors, display names and their profile tables. |
| `include/ubc/core/` | This directory holds shared rules for quantities, selection, recipe jobs and results, together with SHA-256. |
| `include/ubc/engine/` | These headers describe engine interfaces, memory readers, module checks and call sites. |
| `include/ubc/stations/` | Shared station contracts live here, with station-specific behavior in the subdirectories below. |
| `stations/alchemy/` | The alchemy readers check additional recipes, dependencies and flask consumption. |
| `stations/anvil/` | The anvil checks recipe selection, inventory and dialogue flow. |
| `stations/bow/` | The bow bench combines recipe selection with item checks. |
| `stations/cool/` | The cooling trough checks blades, advance debits and station completion. |
| `stations/grind/` | The grindstone checks weapon profiles, previews, consumption and results. |
| `stations/ore/` | Ore recipes have their own selection, runtime and inventory checks. |
| `stations/saw/` | The saw tracks station state and ammunition output. |
| `stations/spirit/` | The spirit potion checks ingredients, conditional output and flask consumption. |
| `stations/tobacco/` | Tobacco recipes check parser state and quantity transactions. |
| `include/ubc/ui/` | This directory holds menu models and native interface declarations. |
| `include/ubc/text/` | This directory defines language selection, text keys and the table format. |
| `include/ubc/text/locales/` | Each supported language supplies its complete translation table here. |

<a id="tables"></a>

## Maintaining recipe tables

The build reads the checked-in catalogs. Maintainers edit the bow catalog directly, for example to correct measured runtime names. The grind profile table, item shapes and constructor data come from an earlier offline extraction. Their headers name the commands used in the original extraction workspace. The `recipe_family_table` test compares all 98 compiled profiles with the checked-in reference rows. [Profile comparison](../tests/recipe_family_table_tests.cpp)
