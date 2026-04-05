cmake_minimum_required(VERSION 3.21)
project(geany_bobui_variant LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Experimental alternate frontend.
# This is intentionally isolated from the GTK-based production build.
# BobUI is consumed via Qt6-compatible package discovery so this variant stays
# toolkit-exclusive while remaining separate from the other experimental frontend tracks.
set(GEANY_BOBUI_HINT_PATHS
    "${CMAKE_CURRENT_SOURCE_DIR}/../../build/bobui-install/lib/cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../build/bobui-install"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../build/bobui"
)

find_package(Qt6 CONFIG QUIET COMPONENTS Core Gui Widgets PATHS ${GEANY_BOBUI_HINT_PATHS})
if (NOT Qt6_FOUND)
    find_package(Qt6 CONFIG REQUIRED COMPONENTS Core Gui Widgets)
endif()

add_executable(geany-bobui-search-studio
    src/main.cpp
    src/search_studio_backend.cpp
)

target_link_libraries(geany-bobui-search-studio
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
)

target_compile_definitions(geany-bobui-search-studio PRIVATE GEANY_BOBUI_EXPERIMENTAL=1)
