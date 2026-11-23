cmake_minimum_required(VERSION 3.21)
# VERSION is the only place that names a release.
file(STRINGS VERSION UBC_VERSION LIMIT_COUNT 1)
if(NOT UBC_VERSION MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)$")
    message(FATAL_ERROR "VERSION must be major.minor.patch, got ${UBC_VERSION}")
endif()
set(UBC_VERSION_COMMAS "${CMAKE_MATCH_1},${CMAKE_MATCH_2},${CMAKE_MATCH_3},0")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS VERSION)
project(UnionBatchCrafting VERSION ${UBC_VERSION} LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
include(cmake/ui_languages.cmake)

add_executable(ubc_core_tests tests/native/core/core_tests.cpp)
target_include_directories(ubc_core_tests PRIVATE include)
add_executable(ubc_sha256_tests tests/native/core/sha256_tests.cpp)
target_include_directories(ubc_sha256_tests PRIVATE include)
add_executable(ubc_menu_model_tests tests/native/ui/menu_model_tests.cpp)
target_include_directories(ubc_menu_model_tests PRIVATE include)
add_executable(
    ubc_station_snapshot_tests
    tests/native/stations/station_snapshot_tests.cpp
)
target_include_directories(ubc_station_snapshot_tests PRIVATE include)
add_executable(
    ubc_runtime_script_tests
    tests/native/engine/runtime_script_tests.cpp
)
target_include_directories(ubc_runtime_script_tests PRIVATE include)
add_executable(
    ubc_hand_snapshot_tests
    tests/native/engine/hand_snapshot_tests.cpp
)
target_include_directories(ubc_hand_snapshot_tests PRIVATE include)
add_executable(
    ubc_info_snapshot_tests
    tests/native/engine/info_snapshot_tests.cpp
)
target_include_directories(ubc_info_snapshot_tests PRIVATE include)
add_executable(
    ubc_packed_inventory_tests
    tests/native/engine/packed_inventory_tests.cpp
)
target_include_directories(ubc_packed_inventory_tests PRIVATE include)
add_executable(
    ubc_selection_session_tests
    tests/native/core/selection_session_tests.cpp
)
target_include_directories(ubc_selection_session_tests PRIVATE include)
add_executable(
    ubc_selection_stock_tests
    tests/native/core/selection_stock_tests.cpp
)
target_include_directories(ubc_selection_stock_tests PRIVATE include)
add_executable(
    ubc_selection_snapshot_tests
    tests/native/core/selection_snapshot_tests.cpp
)
target_include_directories(ubc_selection_snapshot_tests PRIVATE include)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_library(ubc_cool_measurement SHARED tools/world/cool_measurement.cpp)
    target_include_directories(ubc_cool_measurement PRIVATE include)
endif()
enable_testing()
add_library(
    ubc_unrepresentable_game_text_tests
    OBJECT
    EXCLUDE_FROM_ALL
    tests/native/text/unrepresentable.cpp
)
target_include_directories(ubc_unrepresentable_game_text_tests PRIVATE include)
add_test(
    NAME unrepresentable_game_text
    COMMAND
        "${CMAKE_COMMAND}" "-DBUILD=${CMAKE_CURRENT_BINARY_DIR}" -P
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/check_unrepresentable.cmake"
)
add_test(NAME core COMMAND ubc_core_tests)
add_test(NAME sha256 COMMAND ubc_sha256_tests)
add_test(NAME menu_model COMMAND ubc_menu_model_tests)
add_test(NAME station_snapshot COMMAND ubc_station_snapshot_tests)
add_test(NAME runtime_script COMMAND ubc_runtime_script_tests)
add_test(NAME hand_snapshot COMMAND ubc_hand_snapshot_tests)
add_test(NAME info_snapshot COMMAND ubc_info_snapshot_tests)
add_test(NAME packed_inventory COMMAND ubc_packed_inventory_tests)
add_test(NAME selection_session COMMAND ubc_selection_session_tests)
add_test(NAME selection_stock COMMAND ubc_selection_stock_tests)
add_test(NAME selection_snapshot COMMAND ubc_selection_snapshot_tests)

foreach(
    test_path
    engine/player_engine
    core/batch_session
    ui/batch_menu
    engine/amount_patch
    core/batch_result
    catalogs/recipe_catalog
    core/recipe_material
    core/recipe_result
    core/recipe_runtime
    core/recipe_session
    ui/recipe_menu
    core/recipe_navigation
    core/recipe_handoff
    core/recipe_snapshot
    catalogs/station_recipe_catalog
    stations/station_recipe_material
    stations/station_recipe_navigation
    stations/station_recipe_result
    stations/station_recipe_runtime
    stations/station_recipe_snapshot
    core/post_remove_choice
    catalogs/crafting_catalog
    catalogs/recipe_family_table
    stations/tobacco/tobacco_recipe
    core/crafting_session
    ui/crafting_menu
    ui/crafting_ui_diagnostics
    ui/crafting_preview_cache
    stations/grind/grind_ui_readiness
    core/crafting_handoff
    core/crafting_result
    stations/saw/ammo_presentation
    stations/saw/ammo_runtime
    stations/alchemy/alchemy_flask_debit
    stations/station_end_contract
    catalogs/alchemy_recipe_catalog
    stations/alchemy/alchemy_flask_runtime
    stations/alchemy/alchemy_extra_snapshot
    stations/alchemy/alchemy_extra_result
    stations/alchemy/alchemy_extra_navigation
    stations/alchemy/alchemy_extra_execution
    stations/alchemy/alchemy_extra_dependencies
    stations/alchemy/alchemy_extra_dependency_reader
    stations/alchemy/alchemy_extra_end_snapshot
    stations/alchemy/crafting_alchemy_extra_reader
    engine/vob_locator_compat
    engine/vob_inventory_insert_compat
    engine/zbug_end_compat
    catalogs/anvil_recipe_catalog
    stations/anvil/anvil_recipe_runtime
    stations/anvil/anvil_recipe_navigation
    stations/anvil/anvil_recipe_snapshot
    catalogs/ore_recipe_catalog
    stations/ore/ore_recipe_runtime
    stations/ore/ore_recipe_navigation
    stations/ore/ore_recipe_snapshot
    stations/mixed_hotsteel
    catalogs/spirit_recipe_catalog
    stations/spirit/spirit_recipe_runtime
    stations/spirit/spirit_recipe_navigation
    stations/spirit/spirit_recipe_snapshot
    stations/spirit/spirit_execution
    stations/spirit/spirit_result
    stations/spirit/spirit_end_snapshot
    stations/spirit/spirit_crafting_reader
    stations/spirit/crafting_spirit_projection
    stations/bow/crafting_bow_projection
    stations/bow/crafting_bow_execution
    catalogs/bow_recipe_catalog
    stations/bow/bow_recipe_runtime
    stations/bow/bow_recipe_navigation
    stations/bow/bow_recipe_snapshot
    stations/bow/bow_item_path
    stations/bow/bow_item_code
    stations/bow/bow_item_native
    stations/bow/bow_item_renderer
    stations/bow/bow_item_runtime
    stations/grind/grind_recipe_plan
    stations/grind/grind_recipe_material
    stations/grind/grind_recipe_snapshot
    stations/grind/grind_item_binding
    stations/grind/grind_item_path
    stations/grind/grind_item_runtime
    stations/grind/grind_preview_batch
    stations/grind/grind_zbug_unpack
    stations/grind/grind_destroy_observation
    stations/grind/grind_guild_snapshot
    stations/grind/grind_recipe_runtime
    stations/grind/grind_recipe_navigation
    stations/cool/cool_recipe_plan
    stations/cool/cool_recipe_runtime
    stations/cool/cool_recipe_snapshot
    stations/cool/cool_recipe_navigation
    stations/cool/cool_recipe_facts
    stations/cool/cool_parser_prefix
    stations/cool/cool_hotblade_debit
    stations/cool/cool_predebit_contract
    stations/cool/cool_predebit_reader
    stations/cool/cool_nofx_reader
    stations/cool/cool_message_malloc_reader
    stations/cool/cool_direct_contract
    stations/cool/cool_actor_preview
    engine/destroy_remove_callsite
    stations/cool/cool_native_path
    stations/cool/cool_destroy_dispatch
    stations/cool/cool_destroy_observation
)
    get_filename_component(unit "${test_path}" NAME)
    add_executable(ubc_${unit}_tests tests/native/${test_path}_tests.cpp)
    target_include_directories(ubc_${unit}_tests PRIVATE include)
    add_test(NAME ${unit} COMMAND ubc_${unit}_tests)
    if(WIN32)
        set_property(
            TARGET ubc_${unit}_tests
            PROPERTY MSVC_RUNTIME_LIBRARY MultiThreaded
        )
        target_compile_options(ubc_${unit}_tests PRIVATE /W4 /WX)
        target_link_options(ubc_${unit}_tests PRIVATE /Brepro /INCREMENTAL:NO)
    else()
        target_compile_options(
            ubc_${unit}_tests
            PRIVATE -Wall -Wextra -Wpedantic -Werror
        )
    endif()
endforeach()
set_tests_properties(
    recipe_family_table
    PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

add_executable(
    ubc_native_crafting_ui_tests
    tests/native/ui/native_crafting_ui_tests.cpp
    src/ui/native_crafting_ui.cpp
)
target_include_directories(ubc_native_crafting_ui_tests PRIVATE include)
add_test(NAME native_crafting_ui COMMAND ubc_native_crafting_ui_tests)
if(WIN32)
    target_sources(
        ubc_native_crafting_ui_tests
        PRIVATE src/ui/native_selection_ui.cpp
    )
    set_property(
        TARGET ubc_native_crafting_ui_tests
        PROPERTY MSVC_RUNTIME_LIBRARY MultiThreaded
    )
    target_compile_definitions(
        ubc_native_crafting_ui_tests
        PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX
    )
    target_link_libraries(ubc_native_crafting_ui_tests PRIVATE user32)
    target_compile_options(ubc_native_crafting_ui_tests PRIVATE /W4 /WX)
else()
    target_compile_options(
        ubc_native_crafting_ui_tests
        PRIVATE -Wall -Wextra -Wpedantic -Werror
    )
endif()

add_executable(
    ubc_crafting_spirit_shared_tests
    tests/native/stations/spirit/crafting_spirit_shared_tests.cpp
    src/ui/native_crafting_ui.cpp
)
target_include_directories(ubc_crafting_spirit_shared_tests PRIVATE include)
add_test(NAME crafting_spirit_shared COMMAND ubc_crafting_spirit_shared_tests)
if(WIN32)
    target_sources(
        ubc_crafting_spirit_shared_tests
        PRIVATE src/ui/native_selection_ui.cpp
    )
    set_property(
        TARGET ubc_crafting_spirit_shared_tests
        PROPERTY MSVC_RUNTIME_LIBRARY MultiThreaded
    )
    target_compile_definitions(
        ubc_crafting_spirit_shared_tests
        PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX
    )
    target_link_libraries(ubc_crafting_spirit_shared_tests PRIVATE user32)
    target_compile_options(ubc_crafting_spirit_shared_tests PRIVATE /W4 /WX)
else()
    target_compile_options(
        ubc_crafting_spirit_shared_tests
        PRIVATE -Wall -Wextra -Wpedantic -Werror
    )
endif()

add_executable(
    ubc_crafting_cool_shared_tests
    tests/native/stations/cool/crafting_cool_shared_tests.cpp
    src/ui/native_crafting_ui.cpp
)
target_include_directories(ubc_crafting_cool_shared_tests PRIVATE include)
add_test(NAME crafting_cool_shared COMMAND ubc_crafting_cool_shared_tests)
if(WIN32)
    target_sources(
        ubc_crafting_cool_shared_tests
        PRIVATE src/ui/native_selection_ui.cpp
    )
    set_property(
        TARGET ubc_crafting_cool_shared_tests
        PROPERTY MSVC_RUNTIME_LIBRARY MultiThreaded
    )
    target_compile_definitions(
        ubc_crafting_cool_shared_tests
        PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX
    )
    target_link_libraries(ubc_crafting_cool_shared_tests PRIVATE user32)
    target_compile_options(ubc_crafting_cool_shared_tests PRIVATE /W4 /WX)
else()
    target_compile_options(
        ubc_crafting_cool_shared_tests
        PRIVATE -Wall -Wextra -Wpedantic -Werror
    )
endif()

add_executable(
    ubc_crafting_grind_shared_tests
    tests/native/stations/grind/crafting_grind_shared_tests.cpp
    src/ui/native_crafting_ui.cpp
)
target_include_directories(ubc_crafting_grind_shared_tests PRIVATE include)
add_test(NAME crafting_grind_shared COMMAND ubc_crafting_grind_shared_tests)
if(WIN32)
    target_sources(
        ubc_crafting_grind_shared_tests
        PRIVATE src/ui/native_selection_ui.cpp
    )
    set_property(
        TARGET ubc_crafting_grind_shared_tests
        PROPERTY MSVC_RUNTIME_LIBRARY MultiThreaded
    )
    target_compile_definitions(
        ubc_crafting_grind_shared_tests
        PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX
    )
    target_link_libraries(ubc_crafting_grind_shared_tests PRIVATE user32)
    target_compile_options(ubc_crafting_grind_shared_tests PRIVATE /W4 /WX)
else()
    target_compile_options(
        ubc_crafting_grind_shared_tests
        PRIVATE -Wall -Wextra -Wpedantic -Werror
    )
endif()

option(
    UBC_SANITIZE
    "AddressSanitizer und UndefinedBehaviorSanitizer aktivieren"
    OFF
)
if(UBC_SANITIZE AND NOT WIN32)
    target_compile_options(
        ubc_core_tests
        PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer
    )
    target_link_options(ubc_core_tests PRIVATE -fsanitize=address,undefined)
    target_compile_options(
        ubc_sha256_tests
        PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer
    )
    target_link_options(ubc_sha256_tests PRIVATE -fsanitize=address,undefined)
    target_compile_options(
        ubc_menu_model_tests
        PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer
    )
    target_link_options(
        ubc_menu_model_tests
        PRIVATE -fsanitize=address,undefined
    )
    foreach(
        target
        ubc_station_snapshot_tests
        ubc_runtime_script_tests
        ubc_hand_snapshot_tests
        ubc_info_snapshot_tests
        ubc_packed_inventory_tests
        ubc_selection_session_tests
        ubc_selection_stock_tests
        ubc_selection_snapshot_tests
        ubc_batch_session_tests
        ubc_batch_menu_tests
        ubc_amount_patch_tests
        ubc_batch_result_tests
        ubc_recipe_catalog_tests
        ubc_recipe_material_tests
        ubc_recipe_result_tests
        ubc_recipe_runtime_tests
        ubc_recipe_session_tests
        ubc_recipe_menu_tests
        ubc_recipe_navigation_tests
        ubc_recipe_handoff_tests
        ubc_recipe_snapshot_tests
        ubc_station_recipe_catalog_tests
        ubc_station_recipe_material_tests
        ubc_station_recipe_navigation_tests
        ubc_station_recipe_result_tests
        ubc_station_recipe_runtime_tests
        ubc_station_recipe_snapshot_tests
        ubc_post_remove_choice_tests
        ubc_crafting_catalog_tests
        ubc_recipe_family_table_tests
        ubc_tobacco_recipe_tests
        ubc_crafting_session_tests
        ubc_crafting_menu_tests
        ubc_crafting_ui_diagnostics_tests
        ubc_crafting_preview_cache_tests
        ubc_grind_ui_readiness_tests
        ubc_crafting_handoff_tests
        ubc_crafting_result_tests
        ubc_ammo_presentation_tests
        ubc_ammo_runtime_tests
        ubc_alchemy_flask_debit_tests
        ubc_native_crafting_ui_tests
        ubc_station_end_contract_tests
        ubc_alchemy_recipe_catalog_tests
        ubc_alchemy_flask_runtime_tests
        ubc_alchemy_extra_snapshot_tests
        ubc_alchemy_extra_result_tests
        ubc_alchemy_extra_navigation_tests
        ubc_alchemy_extra_execution_tests
        ubc_alchemy_extra_dependencies_tests
        ubc_alchemy_extra_dependency_reader_tests
        ubc_alchemy_extra_end_snapshot_tests
        ubc_crafting_alchemy_extra_reader_tests
        ubc_vob_locator_compat_tests
        ubc_vob_inventory_insert_compat_tests
        ubc_zbug_end_compat_tests
        ubc_anvil_recipe_catalog_tests
        ubc_anvil_recipe_runtime_tests
        ubc_anvil_recipe_navigation_tests
        ubc_anvil_recipe_snapshot_tests
        ubc_ore_recipe_catalog_tests
        ubc_ore_recipe_runtime_tests
        ubc_ore_recipe_navigation_tests
        ubc_ore_recipe_snapshot_tests
        ubc_mixed_hotsteel_tests
        ubc_spirit_recipe_catalog_tests
        ubc_spirit_recipe_runtime_tests
        ubc_spirit_recipe_navigation_tests
        ubc_spirit_recipe_snapshot_tests
        ubc_spirit_execution_tests
        ubc_spirit_result_tests
        ubc_spirit_end_snapshot_tests
        ubc_spirit_crafting_reader_tests
        ubc_crafting_spirit_projection_tests
        ubc_crafting_bow_projection_tests
        ubc_crafting_bow_execution_tests
        ubc_crafting_spirit_shared_tests
        ubc_bow_recipe_catalog_tests
        ubc_bow_recipe_runtime_tests
        ubc_bow_recipe_navigation_tests
        ubc_bow_recipe_snapshot_tests
        ubc_bow_item_path_tests
        ubc_bow_item_code_tests
        ubc_bow_item_native_tests
        ubc_bow_item_renderer_tests
        ubc_bow_item_runtime_tests
        ubc_grind_recipe_plan_tests
        ubc_grind_recipe_material_tests
        ubc_grind_recipe_snapshot_tests
        ubc_grind_item_binding_tests
        ubc_grind_item_path_tests
        ubc_grind_item_runtime_tests
        ubc_grind_preview_batch_tests
        ubc_grind_zbug_unpack_tests
        ubc_grind_destroy_observation_tests
        ubc_grind_guild_snapshot_tests
        ubc_grind_recipe_runtime_tests
        ubc_grind_recipe_navigation_tests
        ubc_cool_recipe_plan_tests
        ubc_cool_recipe_runtime_tests
        ubc_cool_recipe_snapshot_tests
        ubc_cool_recipe_navigation_tests
        ubc_cool_recipe_facts_tests
        ubc_cool_parser_prefix_tests
        ubc_cool_hotblade_debit_tests
        ubc_cool_predebit_contract_tests
        ubc_cool_predebit_reader_tests
        ubc_cool_nofx_reader_tests
        ubc_cool_message_malloc_reader_tests
        ubc_cool_direct_contract_tests
        ubc_cool_actor_preview_tests
        ubc_destroy_remove_callsite_tests
        ubc_cool_native_path_tests
        ubc_cool_destroy_dispatch_tests
        ubc_cool_destroy_observation_tests
        ubc_crafting_cool_shared_tests
        ubc_crafting_grind_shared_tests
    )
        target_compile_options(
            ${target}
            PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer
        )
        target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endforeach()
endif()

if(WIN32)
    if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
        message(FATAL_ERROR "Die Gothic-DLL benötigt Windows x86 (32 Bit).")
    endif()
    set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded)
    set_property(
        TARGET ubc_core_tests
        PROPERTY MSVC_RUNTIME_LIBRARY MultiThreaded
    )
    set_property(
        TARGET ubc_sha256_tests
        PROPERTY MSVC_RUNTIME_LIBRARY MultiThreaded
    )
    set_property(
        TARGET ubc_menu_model_tests
        PROPERTY MSVC_RUNTIME_LIBRARY MultiThreaded
    )
    set_property(
        TARGET ubc_station_snapshot_tests
        PROPERTY MSVC_RUNTIME_LIBRARY MultiThreaded
    )
    set_property(
        TARGET ubc_runtime_script_tests
        PROPERTY MSVC_RUNTIME_LIBRARY MultiThreaded
    )
    set_property(
        TARGET ubc_hand_snapshot_tests
        PROPERTY MSVC_RUNTIME_LIBRARY MultiThreaded
    )
    set_property(
        TARGET ubc_info_snapshot_tests
        PROPERTY MSVC_RUNTIME_LIBRARY MultiThreaded
    )
    set_property(
        TARGET ubc_packed_inventory_tests
        PROPERTY MSVC_RUNTIME_LIBRARY MultiThreaded
    )
    set_property(
        TARGET ubc_selection_session_tests
        PROPERTY MSVC_RUNTIME_LIBRARY MultiThreaded
    )
    set_property(
        TARGET ubc_selection_stock_tests
        PROPERTY MSVC_RUNTIME_LIBRARY MultiThreaded
    )
    set_property(
        TARGET ubc_selection_snapshot_tests
        PROPERTY MSVC_RUNTIME_LIBRARY MultiThreaded
    )
    add_library(
        UnionBatchCraftingProbe
        SHARED
        src/probes/probe.cpp
        src/probes/probe.def
    )
    target_include_directories(UnionBatchCraftingProbe PRIVATE include)
    target_compile_definitions(
        UnionBatchCraftingProbe
        PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX
    )
    target_link_libraries(UnionBatchCraftingProbe PRIVATE kernel32)
    add_executable(ubc_probe_host tests/native/probes/probe_host.cpp)
    target_compile_definitions(
        ubc_probe_host
        PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX
    )
    target_link_libraries(ubc_probe_host PRIVATE kernel32)
    add_library(
        UnionBatchCraftingMenuProbe
        SHARED
        src/probes/native_menu_probe.cpp
        src/probes/native_menu_probe.def
    )
    target_include_directories(UnionBatchCraftingMenuProbe PRIVATE include)
    target_compile_definitions(
        UnionBatchCraftingMenuProbe
        PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX
    )
    target_link_libraries(UnionBatchCraftingMenuProbe PRIVATE kernel32 user32)
    add_executable(ubc_menu_probe_host tests/native/probes/menu_probe_host.cpp)
    target_compile_definitions(
        ubc_menu_probe_host
        PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX
    )
    target_link_libraries(ubc_menu_probe_host PRIVATE kernel32)
    add_library(
        UnionBatchCraftingStationProbe
        SHARED
        src/probes/native_station_probe.cpp
        src/probes/native_station_probe.def
    )
    target_include_directories(UnionBatchCraftingStationProbe PRIVATE include)
    target_compile_definitions(
        UnionBatchCraftingStationProbe
        PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX
    )
    target_link_libraries(UnionBatchCraftingStationProbe PRIVATE kernel32)
    add_library(
        UnionBatchCraftingSelectionProbe
        SHARED
        src/probes/native_selection_probe.cpp
        src/probes/native_selection_probe.def
        src/ui/native_selection_ui.cpp
        src/bridges/frame_bridge.cpp
    )
    target_include_directories(UnionBatchCraftingSelectionProbe PRIVATE include)
    target_compile_definitions(
        UnionBatchCraftingSelectionProbe
        PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX
    )
    target_link_libraries(
        UnionBatchCraftingSelectionProbe
        PRIVATE kernel32 user32
    )
    add_library(
        UnionBatchCrafting
        SHARED
        src/probes/native_batch_probe.cpp
        src/probes/native_batch_probe.def
        src/ui/native_batch_ui.cpp
        src/ui/native_selection_ui.cpp
        src/bridges/frame_bridge.cpp
        src/bridges/batch_call_bridge.cpp
    )
    target_include_directories(UnionBatchCrafting PRIVATE include)
    target_compile_definitions(
        UnionBatchCrafting
        PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX
    )
    target_link_libraries(UnionBatchCrafting PRIVATE kernel32 user32)
    target_compile_options(UnionBatchCrafting PRIVATE /W4 /WX)
    target_link_options(UnionBatchCrafting PRIVATE /Brepro /INCREMENTAL:NO)
    add_library(
        UnionBatchCraftingRecipes
        SHARED
        src/probes/native_recipe_probe.cpp
        src/probes/native_recipe_probe.def
        src/ui/native_recipe_ui.cpp
        src/ui/native_selection_ui.cpp
        src/bridges/frame_bridge.cpp
        src/bridges/batch_call_bridge.cpp
    )
    target_include_directories(UnionBatchCraftingRecipes PRIVATE include)
    target_compile_definitions(
        UnionBatchCraftingRecipes
        PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX
    )
    target_link_libraries(UnionBatchCraftingRecipes PRIVATE kernel32 user32)
    target_compile_options(UnionBatchCraftingRecipes PRIVATE /W4 /WX)
    target_link_options(
        UnionBatchCraftingRecipes
        PRIVATE /Brepro /INCREMENTAL:NO
    )
    set(crafting_runtime_sources
        src/plugin/native_crafting.cpp
        src/plugin/crafting/state.cpp
        src/plugin/crafting/context.cpp
        src/plugin/crafting/player_contracts.cpp
        src/plugin/crafting/stations.cpp
        src/plugin/crafting/cooling.cpp
        src/plugin/crafting/grinding.cpp
        src/plugin/crafting/diagnostics.cpp
        src/plugin/crafting/handoff.cpp
        src/plugin/crafting/menu.cpp
        src/plugin/crafting/amount.cpp
        src/plugin/crafting/tobacco.cpp
        src/plugin/crafting/hooks.cpp
    )
    function(add_crafting_plugin target resource)
        add_library(
            ${target}
            SHARED
            ${crafting_runtime_sources}
            src/plugin/native_crafting.def
            ${resource}
            src/ui/native_crafting_ui.cpp
            src/ui/native_selection_ui.cpp
            src/bridges/frame_bridge.cpp
            src/bridges/batch_call_bridge.cpp
            src/plugin/native_crafting_ammo.cpp
            src/bridges/ammo_presentation_bridge.cpp
            src/writers/alchemy_extra_writer.cpp
            src/bridges/alchemy_flask_debit_bridge.cpp
            src/bridges/cool_predebit_bridge.cpp
            src/writers/spirit_writer.cpp
            src/bridges/spirit_flask_debit_bridge.cpp
        )
        target_include_directories(${target} PRIVATE include)
        target_compile_definitions(
            ${target}
            PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX
        )
        target_link_libraries(${target} PRIVATE kernel32 user32)
        target_compile_options(
            ${target}
            PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:/W4;/WX>"
        )
        target_link_options(${target} PRIVATE /Brepro /INCREMENTAL:NO)
    endfunction()
    option(
        UBC_COOL_WORLD_TEST
        "Getrennter Build für das Kühl-Welttestpaket"
        OFF
    )
    # Both plugins carry the version resource generated from VERSION.
    set(version_resource "${CMAKE_CURRENT_BINARY_DIR}/generated/version.rc")
    configure_file(cmake/version.rc.in "${version_resource}" @ONLY)
    add_crafting_plugin(UnionBatchCraftingExtended "${version_resource}")
    add_crafting_plugin(UnionBatchCraftingPlayer "${version_resource}")
    target_compile_definitions(
        UnionBatchCraftingPlayer
        PRIVATE UBC_PLAYER_RELEASE
    )
    set_target_properties(
        UnionBatchCraftingPlayer
        PROPERTIES
            OUTPUT_NAME UnionBatchCraftingExtended
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/player"
            ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/player"
    )
    add_executable(
        ubc_crafting_effect_guard_tests
        tests/native/engine/crafting_effect_guard_tests.cpp
        src/ui/native_crafting_ui.cpp
        src/ui/native_selection_ui.cpp
        src/bridges/frame_bridge.cpp
        src/bridges/batch_call_bridge.cpp
        src/writers/alchemy_extra_writer.cpp
        src/bridges/alchemy_flask_debit_bridge.cpp
        src/bridges/cool_predebit_bridge.cpp
        src/writers/spirit_writer.cpp
        src/bridges/spirit_flask_debit_bridge.cpp
    )
    target_include_directories(ubc_crafting_effect_guard_tests PRIVATE include)
    target_compile_definitions(
        ubc_crafting_effect_guard_tests
        PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX
    )
    target_link_libraries(
        ubc_crafting_effect_guard_tests
        PRIVATE kernel32 user32
    )
    target_compile_options(ubc_crafting_effect_guard_tests PRIVATE /W4 /WX)
    target_link_options(
        ubc_crafting_effect_guard_tests
        PRIVATE /Brepro /INCREMENTAL:NO
    )
    add_executable(
        ubc_native_crafting_ammo_tests
        tests/native/engine/native_crafting_ammo_tests.cpp
        src/bridges/ammo_presentation_bridge.cpp
    )
    target_include_directories(
        ubc_native_crafting_ammo_tests
        PRIVATE include src
    )
    target_compile_definitions(
        ubc_native_crafting_ammo_tests
        PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX
    )
    target_link_libraries(ubc_native_crafting_ammo_tests PRIVATE kernel32)
    target_compile_options(ubc_native_crafting_ammo_tests PRIVATE /W4 /WX)
    target_link_options(
        ubc_native_crafting_ammo_tests
        PRIVATE /Brepro /INCREMENTAL:NO
    )
    add_executable(
        ubc_crafting_probe_host
        tests/native/probes/crafting_probe_host.cpp
    )
    target_compile_definitions(
        ubc_crafting_probe_host
        PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX
    )
    target_link_libraries(ubc_crafting_probe_host PRIVATE kernel32)
    target_compile_options(ubc_crafting_probe_host PRIVATE /W4 /WX)
    target_link_options(ubc_crafting_probe_host PRIVATE /Brepro /INCREMENTAL:NO)
    add_executable(
        ubc_recipe_probe_host
        tests/native/probes/recipe_probe_host.cpp
    )
    target_compile_definitions(
        ubc_recipe_probe_host
        PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX
    )
    target_link_libraries(ubc_recipe_probe_host PRIVATE kernel32)
    target_compile_options(ubc_recipe_probe_host PRIVATE /W4 /WX)
    target_link_options(ubc_recipe_probe_host PRIVATE /Brepro /INCREMENTAL:NO)
    add_executable(
        ubc_recipe_effect_guard_tests
        tests/native/engine/recipe_effect_guard_tests.cpp
        src/ui/native_recipe_ui.cpp
        src/ui/native_selection_ui.cpp
        src/bridges/frame_bridge.cpp
        src/bridges/batch_call_bridge.cpp
    )
    target_include_directories(ubc_recipe_effect_guard_tests PRIVATE include)
    target_compile_definitions(
        ubc_recipe_effect_guard_tests
        PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX
    )
    target_link_libraries(ubc_recipe_effect_guard_tests PRIVATE kernel32 user32)
    target_compile_options(ubc_recipe_effect_guard_tests PRIVATE /W4 /WX)
    target_link_options(
        ubc_recipe_effect_guard_tests
        PRIVATE /Brepro /INCREMENTAL:NO
    )
    add_executable(
        ubc_batch_call_bridge_tests
        tests/native/engine/batch_call_bridge_tests.cpp
        src/bridges/batch_call_bridge.cpp
    )
    target_include_directories(ubc_batch_call_bridge_tests PRIVATE include)
    target_compile_definitions(
        ubc_batch_call_bridge_tests
        PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX
    )
    target_link_libraries(ubc_batch_call_bridge_tests PRIVATE kernel32)
    target_compile_options(ubc_batch_call_bridge_tests PRIVATE /W4 /WX)
    target_link_options(
        ubc_batch_call_bridge_tests
        PRIVATE /Brepro /INCREMENTAL:NO
    )
    add_executable(
        ubc_ammo_presentation_bridge_tests
        tests/native/ui/ammo_presentation_bridge_tests.cpp
        src/bridges/ammo_presentation_bridge.cpp
    )
    target_include_directories(
        ubc_ammo_presentation_bridge_tests
        PRIVATE include src
    )
    target_compile_definitions(
        ubc_ammo_presentation_bridge_tests
        PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX
    )
    target_link_libraries(ubc_ammo_presentation_bridge_tests PRIVATE kernel32)
    target_compile_options(ubc_ammo_presentation_bridge_tests PRIVATE /W4 /WX)
    target_link_options(
        ubc_ammo_presentation_bridge_tests
        PRIVATE /Brepro /INCREMENTAL:NO
    )
    add_executable(
        ubc_alchemy_flask_debit_bridge_tests
        tests/native/stations/alchemy/alchemy_flask_debit_bridge_tests.cpp
        src/bridges/alchemy_flask_debit_bridge.cpp
    )
    target_include_directories(
        ubc_alchemy_flask_debit_bridge_tests
        PRIVATE include src
    )
    target_compile_definitions(
        ubc_alchemy_flask_debit_bridge_tests
        PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX
    )
    target_link_libraries(ubc_alchemy_flask_debit_bridge_tests PRIVATE kernel32)
    target_compile_options(ubc_alchemy_flask_debit_bridge_tests PRIVATE /W4 /WX)
    target_link_options(
        ubc_alchemy_flask_debit_bridge_tests
        PRIVATE /Brepro /INCREMENTAL:NO
    )
    # Offline integration for Lager2/N2. Deliberately omit the plugin link until EndFence/Reader are available.
    add_executable(
        ubc_cool_hotblade_debit_bridge_tests
        tests/native/stations/cool/cool_hotblade_debit_bridge_tests.cpp
        src/bridges/cool_hotblade_debit_bridge.cpp
    )
    target_include_directories(
        ubc_cool_hotblade_debit_bridge_tests
        PRIVATE include src
    )
    target_compile_definitions(
        ubc_cool_hotblade_debit_bridge_tests
        PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX
    )
    target_link_libraries(ubc_cool_hotblade_debit_bridge_tests PRIVATE kernel32)
    target_compile_options(ubc_cool_hotblade_debit_bridge_tests PRIVATE /W4 /WX)
    target_link_options(
        ubc_cool_hotblade_debit_bridge_tests
        PRIVATE /Brepro /INCREMENTAL:NO
    )
    add_library(
        ubc_cool_predebit_test_host
        SHARED
        tests/native/stations/cool/cool_predebit_bridge_tests.cpp
        tests/native/stations/cool/cool_predebit_native_host.cpp
        src/ui/native_crafting_ui.cpp
        src/ui/native_selection_ui.cpp
        src/bridges/frame_bridge.cpp
        src/bridges/batch_call_bridge.cpp
        src/plugin/native_crafting_ammo.cpp
        src/bridges/ammo_presentation_bridge.cpp
        src/writers/alchemy_extra_writer.cpp
        src/bridges/alchemy_flask_debit_bridge.cpp
        src/bridges/cool_predebit_bridge.cpp
        src/writers/spirit_writer.cpp
        src/bridges/spirit_flask_debit_bridge.cpp
    )
    target_include_directories(ubc_cool_predebit_test_host PRIVATE include src)
    target_compile_definitions(
        ubc_cool_predebit_test_host
        PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UBC_COOL_PREDEBIT_HOST_TEST
    )
    target_link_libraries(ubc_cool_predebit_test_host PRIVATE kernel32 user32)
    target_compile_options(ubc_cool_predebit_test_host PRIVATE /W4 /WX)
    target_link_options(
        ubc_cool_predebit_test_host
        PRIVATE /Brepro /INCREMENTAL:NO /BASE:0x54000000
    )
    # The test executable owns the fixed call pages at load time. As in
    # the plugin, the actual adapter functions reside outside this image.
    add_executable(
        ubc_cool_predebit_bridge_tests
        tests/native/stations/cool/cool_predebit_host_main.cpp
    )
    target_link_libraries(
        ubc_cool_predebit_bridge_tests
        PRIVATE ubc_cool_predebit_test_host
    )
    target_compile_options(ubc_cool_predebit_bridge_tests PRIVATE /W4 /WX)
    target_link_options(
        ubc_cool_predebit_bridge_tests
        PRIVATE /Brepro /INCREMENTAL:NO /FIXED /DYNAMICBASE:NO /BASE:0x00500000
    )
    add_executable(
        ubc_alchemy_extra_writer_tests
        tests/native/stations/alchemy/alchemy_extra_writer_tests.cpp
        src/writers/alchemy_extra_writer.cpp
        src/bridges/alchemy_flask_debit_bridge.cpp
    )
    target_include_directories(
        ubc_alchemy_extra_writer_tests
        PRIVATE include src
    )
    target_compile_definitions(
        ubc_alchemy_extra_writer_tests
        PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UBC_ALCHEMY_EXTRA_HOST_TEST
    )
    target_link_libraries(ubc_alchemy_extra_writer_tests PRIVATE kernel32)
    target_compile_options(ubc_alchemy_extra_writer_tests PRIVATE /W4 /WX)
    target_link_options(
        ubc_alchemy_extra_writer_tests
        PRIVATE /Brepro /INCREMENTAL:NO
    )
    add_executable(
        ubc_spirit_writer_tests
        tests/native/stations/spirit/spirit_writer_tests.cpp
        src/writers/spirit_writer.cpp
        src/bridges/spirit_flask_debit_bridge.cpp
    )
    target_include_directories(ubc_spirit_writer_tests PRIVATE include src)
    target_compile_definitions(
        ubc_spirit_writer_tests
        PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UBC_SPIRIT_HOST_TEST
    )
    target_link_libraries(ubc_spirit_writer_tests PRIVATE kernel32)
    target_compile_options(ubc_spirit_writer_tests PRIVATE /W4 /WX)
    target_link_options(ubc_spirit_writer_tests PRIVATE /Brepro /INCREMENTAL:NO)
    add_executable(
        ubc_windows_destroy_remove_callsite_tests
        tests/native/engine/windows_destroy_remove_callsite_tests.cpp
    )
    target_include_directories(
        ubc_windows_destroy_remove_callsite_tests
        PRIVATE include
    )
    target_compile_definitions(
        ubc_windows_destroy_remove_callsite_tests
        PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX
    )
    target_link_libraries(
        ubc_windows_destroy_remove_callsite_tests
        PRIVATE kernel32
    )
    target_compile_options(
        ubc_windows_destroy_remove_callsite_tests
        PRIVATE /W4 /WX /GS-
    )
    target_link_options(
        ubc_windows_destroy_remove_callsite_tests
        PRIVATE /Brepro /INCREMENTAL:NO /OPT:NOICF /DYNAMICBASE:NO /FIXED
    )
    add_executable(
        ubc_windows_module_identity_tests
        tests/native/engine/windows_module_identity_tests.cpp
    )
    target_include_directories(
        ubc_windows_module_identity_tests
        PRIVATE include
    )
    target_compile_definitions(
        ubc_windows_module_identity_tests
        PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX
    )
    target_link_libraries(ubc_windows_module_identity_tests PRIVATE kernel32)
    target_compile_options(ubc_windows_module_identity_tests PRIVATE /W4 /WX)
    target_link_options(
        ubc_windows_module_identity_tests
        PRIVATE /Brepro /INCREMENTAL:NO
    )
    add_executable(
        ubc_batch_probe_host
        tests/native/probes/batch_probe_host.cpp
    )
    target_compile_definitions(
        ubc_batch_probe_host
        PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX
    )
    target_link_libraries(ubc_batch_probe_host PRIVATE kernel32)
    target_compile_options(ubc_batch_probe_host PRIVATE /W4 /WX)
    target_link_options(ubc_batch_probe_host PRIVATE /Brepro /INCREMENTAL:NO)
    add_executable(
        ubc_selection_probe_host
        tests/native/probes/selection_probe_host.cpp
    )
    target_compile_definitions(
        ubc_selection_probe_host
        PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX
    )
    target_link_libraries(ubc_selection_probe_host PRIVATE kernel32)
    add_executable(
        ubc_station_probe_host
        tests/native/probes/station_probe_host.cpp
    )
    target_compile_definitions(
        ubc_station_probe_host
        PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX
    )
    target_link_libraries(ubc_station_probe_host PRIVATE kernel32)
    add_executable(
        ubc_native_hook_tests
        tests/native/engine/native_hook_tests.cpp
    )
    target_include_directories(ubc_native_hook_tests PRIVATE include)
    target_compile_definitions(
        ubc_native_hook_tests
        PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX
    )
    target_link_libraries(ubc_native_hook_tests PRIVATE kernel32)
    target_link_options(
        ubc_native_hook_tests
        PRIVATE /BASE:0x400000 /DYNAMICBASE:NO /FIXED /OPT:NOICF
    )
    add_executable(
        ubc_frame_bridge_tests
        tests/native/engine/frame_bridge_tests.cpp
        src/bridges/frame_bridge.cpp
    )
    target_include_directories(ubc_frame_bridge_tests PRIVATE include)
    target_compile_definitions(
        ubc_frame_bridge_tests
        PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX
    )
    target_link_libraries(ubc_frame_bridge_tests PRIVATE kernel32)
    target_link_options(
        ubc_frame_bridge_tests
        PRIVATE /BASE:0x400000 /DYNAMICBASE:NO /FIXED /OPT:NOICF
    )
    add_executable(
        ubc_selection_effect_guard_tests
        tests/native/engine/selection_effect_guard_tests.cpp
        src/ui/native_selection_ui.cpp
        src/bridges/frame_bridge.cpp
    )
    target_include_directories(ubc_selection_effect_guard_tests PRIVATE include)
    target_compile_definitions(
        ubc_selection_effect_guard_tests
        PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX
    )
    target_link_libraries(
        ubc_selection_effect_guard_tests
        PRIVATE kernel32 user32
    )
    set_property(
        TARGET ubc_selection_effect_guard_tests
        PROPERTY MSVC_RUNTIME_LIBRARY MultiThreaded
    )
    target_compile_options(ubc_selection_effect_guard_tests PRIVATE /W4 /WX)
    target_link_options(
        ubc_selection_effect_guard_tests
        PRIVATE /Brepro /INCREMENTAL:NO
    )
    foreach(
        target
        UnionBatchCraftingProbe
        ubc_probe_host
        ubc_core_tests
        ubc_sha256_tests
        ubc_menu_model_tests
        UnionBatchCraftingMenuProbe
        ubc_menu_probe_host
        ubc_station_snapshot_tests
        ubc_runtime_script_tests
        ubc_hand_snapshot_tests
        UnionBatchCraftingStationProbe
        ubc_station_probe_host
        ubc_native_hook_tests
        ubc_info_snapshot_tests
        ubc_packed_inventory_tests
        ubc_selection_session_tests
        ubc_selection_stock_tests
        ubc_frame_bridge_tests
        ubc_selection_snapshot_tests
        UnionBatchCraftingSelectionProbe
        ubc_selection_probe_host
    )
        target_compile_options(${target} PRIVATE /W4)
        target_link_options(${target} PRIVATE /Brepro /INCREMENTAL:NO)
    endforeach()
else()
    target_compile_options(
        ubc_core_tests
        PRIVATE -Wall -Wextra -Wpedantic -Werror
    )
    target_compile_options(
        ubc_sha256_tests
        PRIVATE -Wall -Wextra -Wpedantic -Werror
    )
    target_compile_options(
        ubc_menu_model_tests
        PRIVATE -Wall -Wextra -Wpedantic -Werror
    )
    target_compile_options(
        ubc_station_snapshot_tests
        PRIVATE -Wall -Wextra -Wpedantic -Werror
    )
    target_compile_options(
        ubc_runtime_script_tests
        PRIVATE -Wall -Wextra -Wpedantic -Werror
    )
    target_compile_options(
        ubc_hand_snapshot_tests
        PRIVATE -Wall -Wextra -Wpedantic -Werror
    )
    target_compile_options(
        ubc_info_snapshot_tests
        PRIVATE -Wall -Wextra -Wpedantic -Werror
    )
    target_compile_options(
        ubc_packed_inventory_tests
        PRIVATE -Wall -Wextra -Wpedantic -Werror
    )
    target_compile_options(
        ubc_selection_session_tests
        PRIVATE -Wall -Wextra -Wpedantic -Werror
    )
    target_compile_options(
        ubc_selection_stock_tests
        PRIVATE -Wall -Wextra -Wpedantic -Werror
    )
    target_compile_options(
        ubc_selection_snapshot_tests
        PRIVATE -Wall -Wextra -Wpedantic -Werror
    )
endif()

# The first source of each registered target is its native test translation unit.
get_property(native_tests DIRECTORY PROPERTY TESTS)
foreach(unit IN LISTS native_tests)
    get_target_property(sources ubc_${unit}_tests SOURCES)
    list(GET sources 0 source)
    get_filename_component(directory "${source}" DIRECTORY)
    get_filename_component(area "${directory}" NAME)
    if(area STREQUAL "cool")
        set(area cooling)
    endif()
    set_tests_properties(${unit} PROPERTIES LABELS "${area}")
endforeach()
set_property(TEST native_crafting_ui APPEND PROPERTY LABELS text)
