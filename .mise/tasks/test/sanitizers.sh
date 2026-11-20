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
  -Wall -Wextra -Werror -Wconversion -pedantic -I include -I "$language_include" tests/core_tests.cpp \
  -o "$test_root/core_tests"
"$test_root/core_tests" "$@"
clang++ -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  -Wall -Wextra -Werror -Wconversion -pedantic -I include -I "$language_include" tests/sha256_tests.cpp \
  -o "$test_root/sha256_tests"
"$test_root/sha256_tests"
clang++ -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  -Wall -Wextra -Werror -Wconversion -pedantic -I include -I "$language_include" tests/menu_model_tests.cpp \
  -o "$test_root/menu_model_tests"
"$test_root/menu_model_tests"
for unit in station_snapshot runtime_script hand_snapshot info_snapshot packed_inventory selection_session selection_stock selection_snapshot batch_session batch_menu amount_patch batch_result recipe_catalog recipe_material recipe_result \
  recipe_runtime recipe_session recipe_menu recipe_navigation recipe_handoff recipe_snapshot \
  station_recipe_catalog station_recipe_material station_recipe_navigation station_recipe_result \
  station_recipe_runtime station_recipe_snapshot post_remove_choice \
  crafting_catalog recipe_family_table tobacco_recipe crafting_session crafting_menu crafting_ui_diagnostics grind_ui_readiness crafting_handoff crafting_result ammo_presentation ammo_runtime alchemy_flask_debit station_end_contract alchemy_recipe_catalog alchemy_flask_runtime alchemy_extra_snapshot alchemy_extra_result alchemy_extra_navigation alchemy_extra_execution \
  alchemy_extra_dependencies alchemy_extra_dependency_reader alchemy_extra_end_snapshot crafting_alchemy_extra_reader vob_locator_compat vob_inventory_insert_compat zbug_end_compat \
  anvil_recipe_catalog anvil_recipe_runtime anvil_recipe_navigation anvil_recipe_snapshot \
  ore_recipe_catalog ore_recipe_runtime ore_recipe_navigation ore_recipe_snapshot mixed_hotsteel \
  spirit_recipe_catalog spirit_recipe_runtime spirit_recipe_navigation spirit_recipe_snapshot \
  spirit_execution spirit_result spirit_end_snapshot spirit_crafting_reader crafting_spirit_projection crafting_bow_projection crafting_bow_execution \
  bow_recipe_catalog bow_recipe_runtime bow_recipe_navigation bow_recipe_snapshot \
  bow_item_path bow_item_code bow_item_native bow_item_renderer bow_item_runtime grind_recipe_plan grind_recipe_material grind_recipe_snapshot grind_item_binding grind_item_path grind_item_runtime grind_preview_batch grind_zbug_unpack grind_destroy_observation grind_guild_snapshot grind_recipe_runtime grind_recipe_navigation cool_recipe_plan cool_recipe_runtime cool_recipe_snapshot cool_recipe_navigation cool_recipe_facts cool_parser_prefix cool_hotblade_debit cool_predebit_contract cool_predebit_reader cool_nofx_reader cool_message_malloc_reader cool_direct_contract cool_actor_preview destroy_remove_callsite cool_native_path cool_destroy_dispatch cool_destroy_observation; do
  clang++ -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    -Wall -Wextra -Werror -Wconversion -pedantic -I include -I "$language_include" "tests/${unit}_tests.cpp" \
    -o "$test_root/${unit}_tests"
  "$test_root/${unit}_tests"
  if [[ "$unit" == cool_predebit_reader && "${#cool_files[@]}" -eq 7 ]]; then
    "$test_root/${unit}_tests" --local-files "${cool_files[@]}"
  fi
done
clang++ -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  -Wall -Wextra -Werror -Wconversion -pedantic -I include -I "$language_include" tests/native_crafting_ui_tests.cpp src/ui/native_crafting_ui.cpp \
  -o "$test_root/native_crafting_ui_tests"
"$test_root/native_crafting_ui_tests"
clang++ -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  -Wall -Wextra -Werror -Wconversion -pedantic -I include -I "$language_include" tests/crafting_spirit_shared_tests.cpp src/ui/native_crafting_ui.cpp \
  -o "$test_root/crafting_spirit_shared_tests"
"$test_root/crafting_spirit_shared_tests"
clang++ -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  -Wall -Wextra -Werror -Wconversion -pedantic -I include -I "$language_include" tests/crafting_cool_shared_tests.cpp src/ui/native_crafting_ui.cpp \
  -o "$test_root/crafting_cool_shared_tests"
"$test_root/crafting_cool_shared_tests"

clang++ -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  -Wall -Wextra -Werror -Wconversion -pedantic -I include -I "$language_include" tests/crafting_grind_shared_tests.cpp src/ui/native_crafting_ui.cpp \
  -o "$test_root/crafting_grind_shared_tests"
"$test_root/crafting_grind_shared_tests"
