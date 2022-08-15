cmake_minimum_required(VERSION 3.15)

project(ICEmuPlugins LANGUAGES CXX)

#Include Unicorn
include_directories(${CMAKE_SOURCE_DIR}/../icemu/lib/unicorn/include)
link_directories(${CMAKE_SOURCE_DIR}/../icemu/lib/unicorn)

#Include Capstone
include_directories(${CMAKE_SOURCE_DIR}/../icemu/lib/capstone/include)
link_directories(${CMAKE_SOURCE_DIR}/../icemu/lib/capstone)

#Include JsonCPP
include_directories(${CMAKE_SOURCE_DIR}/../icemu/lib/jsoncpp/include)
link_directories(${CMAKE_SOURCE_DIR}/../icemu/lib/jsoncpp/build/lib)

#Include ElfIO
include_directories(${CMAKE_SOURCE_DIR}/../icemu/lib/ELFIO)

#Include ICEmu
include_directories(${CMAKE_SOURCE_DIR}/../icemu/include)

set(PLUGIN_COMPILE_OPTIONS
    -Wall
    -Wextra
    -g
    -O3
    -std=c++11
    -shared
    -fPIC
    )

set(PLUGIN_LINK_OPTIONS
    -shared
    -fPIC
    )

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/plugins)
include_directories(${PROJECT_SOURCE_DIR}/../icemu/plugins/common)

# add_subdirectory(external_test_plugin)
# add_subdirectory(cache_lru_plugin)
# add_subdirectory(wrw_detection_plugin)
# add_subdirectory(stack_war_detection_plugin)
add_subdirectory(simple_war_detect_plugin)
# add_subdirectory(cache_utilization_plugin)
# add_subdirectory(cache_dead_block_plugin)
# add_subdirectory(cache_temp_dead_block_plugin)
add_subdirectory(clank_cache_plugin)
add_subdirectory(custom_cache_plugin)

