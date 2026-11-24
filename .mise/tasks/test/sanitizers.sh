#!/usr/bin/env bash
#MISE description="Test the core with ASan/UBSan in a temporary directory"
set -euo pipefail
cool_files=()
if [[ "${1:-}" == --cool-local-files ]]; then
  [[ "$#" -eq 8 ]] || {
    echo "Sieben lokale Kühl-Vertragsdateien erforderlich" >&2
    exit 2
  }
  shift
  cool_files=("$@")
  shift 7
fi
test_root=$(mktemp -d /tmp/gothic-batch-test.XXXXXX)
cleanup() {
  case "$test_root" in
    /tmp/gothic-batch-test.*) rm -rf -- "$test_root" ;;
    *) return 1 ;;
  esac
}
trap cleanup EXIT
cmake -S . -B "$test_root/configure" -G Ninja
language_include="$test_root/configure/generated"
clang++ -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  -Wall -Wextra -Werror -Wconversion -pedantic -I include -I "$language_include" tests/native/core/core_tests.cpp \
  -o "$test_root/core_tests"
"$test_root/core_tests" "$@"
clang++ -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  -Wall -Wextra -Werror -Wconversion -pedantic -I include -I "$language_include" tests/native/core/sha256_tests.cpp \
  -o "$test_root/sha256_tests"
"$test_root/sha256_tests"
clang++ -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  -Wall -Wextra -Werror -Wconversion -pedantic -I include -I "$language_include" tests/native/ui/menu_model_tests.cpp \
  -o "$test_root/menu_model_tests"
"$test_root/menu_model_tests"
for test_path in stations/station_snapshot engine/runtime_script engine/player_campaign stations/alchemy/dndr_alchemy engine/hand_snapshot engine/info_snapshot engine/packed_inventory core/selection_session core/selection_stock core/selection_snapshot core/batch_session ui/batch_menu engine/amount_patch core/batch_result catalogs/recipe_catalog core/recipe_material core/recipe_result \
  core/recipe_runtime core/recipe_session ui/recipe_menu core/recipe_navigation core/recipe_handoff core/recipe_snapshot \
  catalogs/station_recipe_catalog stations/station_recipe_material stations/station_recipe_navigation stations/station_recipe_result \
  stations/station_recipe_runtime stations/station_recipe_snapshot core/post_remove_choice \
  catalogs/crafting_catalog catalogs/recipe_family_table stations/tobacco/tobacco_recipe core/crafting_session ui/crafting_menu ui/crafting_ui_diagnostics ui/crafting_preview_cache stations/grind/grind_ui_readiness core/crafting_handoff core/crafting_result stations/saw/ammo_presentation stations/saw/ammo_runtime stations/alchemy/alchemy_flask_debit stations/station_end_contract catalogs/alchemy_recipe_catalog stations/alchemy/alchemy_flask_runtime stations/alchemy/alchemy_extra_snapshot stations/alchemy/alchemy_extra_result stations/alchemy/alchemy_extra_navigation stations/alchemy/alchemy_extra_execution \
  stations/alchemy/alchemy_extra_dependencies stations/alchemy/alchemy_extra_dependency_reader stations/alchemy/alchemy_extra_end_snapshot stations/alchemy/crafting_alchemy_extra_reader engine/vob_locator_compat engine/vob_inventory_insert_compat engine/zbug_end_compat \
  catalogs/anvil_recipe_catalog stations/anvil/anvil_recipe_runtime stations/anvil/anvil_recipe_navigation stations/anvil/anvil_recipe_snapshot \
  catalogs/ore_recipe_catalog stations/ore/ore_recipe_runtime stations/ore/ore_recipe_navigation stations/ore/ore_recipe_snapshot stations/mixed_hotsteel \
  catalogs/spirit_recipe_catalog stations/spirit/spirit_recipe_runtime stations/spirit/spirit_recipe_navigation stations/spirit/spirit_recipe_snapshot \
  stations/spirit/spirit_execution stations/spirit/spirit_result stations/spirit/spirit_end_snapshot stations/spirit/spirit_crafting_reader stations/spirit/crafting_spirit_projection stations/bow/crafting_bow_projection stations/bow/crafting_bow_execution \
  catalogs/bow_recipe_catalog stations/bow/bow_recipe_runtime stations/bow/bow_recipe_navigation stations/bow/bow_recipe_snapshot \
  stations/bow/bow_item_path stations/bow/bow_item_code stations/bow/bow_item_native stations/bow/bow_item_renderer stations/bow/bow_item_runtime stations/grind/grind_recipe_plan stations/grind/grind_recipe_material stations/grind/grind_recipe_snapshot stations/grind/grind_item_binding stations/grind/grind_item_path stations/grind/grind_item_runtime stations/grind/grind_preview_batch stations/grind/grind_zbug_unpack stations/grind/grind_destroy_observation stations/grind/grind_guild_snapshot stations/grind/grind_recipe_runtime stations/grind/grind_recipe_navigation stations/cool/cool_recipe_plan stations/cool/cool_recipe_runtime stations/cool/cool_recipe_snapshot stations/cool/cool_recipe_navigation stations/cool/cool_recipe_facts stations/cool/cool_parser_prefix stations/cool/cool_hotblade_debit stations/cool/cool_predebit_contract stations/cool/cool_predebit_reader stations/cool/cool_nofx_reader stations/cool/cool_message_malloc_reader stations/cool/cool_direct_contract stations/cool/cool_actor_preview engine/destroy_remove_callsite stations/cool/cool_native_path stations/cool/cool_destroy_dispatch stations/cool/cool_destroy_observation; do
  unit=${test_path##*/}
  clang++ -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    -Wall -Wextra -Werror -Wconversion -pedantic -I include -I "$language_include" "tests/native/${test_path}_tests.cpp" \
    -o "$test_root/${unit}_tests"
  "$test_root/${unit}_tests"
  if [[ "$unit" == cool_predebit_reader && "${#cool_files[@]}" -eq 7 ]]; then
    "$test_root/${unit}_tests" --local-files "${cool_files[@]}"
  fi
done
clang++ -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  -Wall -Wextra -Werror -Wconversion -pedantic -I include -I "$language_include" tests/native/ui/native_crafting_ui_tests.cpp src/ui/native_crafting_ui.cpp \
  -o "$test_root/native_crafting_ui_tests"
"$test_root/native_crafting_ui_tests"
clang++ -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  -Wall -Wextra -Werror -Wconversion -pedantic -I include -I "$language_include" tests/native/stations/spirit/crafting_spirit_shared_tests.cpp src/ui/native_crafting_ui.cpp \
  -o "$test_root/crafting_spirit_shared_tests"
"$test_root/crafting_spirit_shared_tests"
clang++ -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  -Wall -Wextra -Werror -Wconversion -pedantic -I include -I "$language_include" tests/native/stations/cool/crafting_cool_shared_tests.cpp src/ui/native_crafting_ui.cpp \
  -o "$test_root/crafting_cool_shared_tests"
"$test_root/crafting_cool_shared_tests"

clang++ -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  -Wall -Wextra -Werror -Wconversion -pedantic -I include -I "$language_include" tests/native/stations/grind/crafting_grind_shared_tests.cpp src/ui/native_crafting_ui.cpp \
  -o "$test_root/crafting_grind_shared_tests"
"$test_root/crafting_grind_shared_tests"
