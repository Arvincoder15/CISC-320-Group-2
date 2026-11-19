# Standalone optional UI prototype: does not change the team's root build.
cmake_minimum_required(VERSION 3.20)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
project(ElementalUIPrototype LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(GROUP2_UI_STANDALONE ON)
set(GROUP2_BUILD_SDL_RENDERER ON CACHE BOOL "Build rendering backend" FORCE)
add_subdirectory(../.. engine-build)
endif()
add_executable(ui_prototype main.cpp)
target_link_libraries(ui_prototype PRIVATE engine_sdl_rendering elemental_coop_game engine_infrastructure engine_tools engine_ai)
group2_target(ui_prototype)
add_custom_target(ui_prototype_assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:ui_prototype>/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/../../assets/rendering $<TARGET_FILE_DIR:ui_prototype>/rendering-assets
    VERBATIM)
add_dependencies(ui_prototype ui_prototype_assets)
include(CTest)
if(BUILD_TESTING)
    add_executable(test_ui_level level_test.cpp)
    target_link_libraries(test_ui_level PRIVATE elemental_coop_game engine_tools engine_ai)
    target_compile_definitions(test_ui_level PRIVATE
        UI_PROTOTYPE_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets")
    group2_target(test_ui_level)
    add_executable(test_ui_enemies enemy_test.cpp)
    target_link_libraries(test_ui_enemies PRIVATE elemental_coop_game engine_tools engine_ai)
    target_include_directories(test_ui_enemies PRIVATE ../../tests)
    target_compile_definitions(test_ui_enemies PRIVATE
        UI_PROTOTYPE_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets")
    group2_target(test_ui_enemies)
    add_test(NAME ui_enemies COMMAND test_ui_enemies)
    add_test(NAME ui_level COMMAND test_ui_level)
    add_test(NAME ui_game_smoke COMMAND ui_prototype --smoke-test)
    set_tests_properties(ui_game_smoke PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
endif()
