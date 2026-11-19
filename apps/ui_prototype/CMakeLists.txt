# Standalone optional UI prototype: does not change the team's root build.
cmake_minimum_required(VERSION 3.20)
project(ElementalUIPrototype LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(SDL3 3.2 CONFIG REQUIRED)
add_subdirectory(../.. engine-build)
add_executable(ui_prototype main.cpp)
target_link_libraries(ui_prototype PRIVATE SDL3::SDL3 elemental_coop_game engine_infrastructure engine_tools)
group2_target(ui_prototype)
add_custom_target(ui_prototype_assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:ui_prototype>/assets
    VERBATIM)
add_dependencies(ui_prototype ui_prototype_assets)
include(CTest)
if(BUILD_TESTING)
    add_executable(test_ui_level level_test.cpp)
    target_link_libraries(test_ui_level PRIVATE elemental_coop_game engine_tools)
    target_compile_definitions(test_ui_level PRIVATE
        UI_PROTOTYPE_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets")
    group2_target(test_ui_level)
    add_test(NAME ui_level COMMAND test_ui_level)
endif()
