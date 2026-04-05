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
    "${CMAKE_CURRENT_SOURCE_DIR}/../../build/bobui-install/lib/cmake/Qt6"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../build/bobui-install"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../build/bobui/lib/cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../build/bobui/lib/cmake/Qt6"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../build/bobui"
)

find_package(Qt6 CONFIG QUIET COMPONENTS Core Gui Widgets PATHS ${GEANY_BOBUI_HINT_PATHS})
if (NOT Qt6_FOUND)
    find_package(Qt6 CONFIG REQUIRED COMPONENTS Core Gui Widgets)
endif()

set(GEANY_BOBUI_RUNTIME_DIR "")
if (DEFINED Qt6_DIR)
    if (EXISTS "${Qt6_DIR}/../../../bin")
        get_filename_component(GEANY_BOBUI_RUNTIME_DIR "${Qt6_DIR}/../../../bin" ABSOLUTE)
    elseif (EXISTS "${Qt6_DIR}/../../bin")
        get_filename_component(GEANY_BOBUI_RUNTIME_DIR "${Qt6_DIR}/../../bin" ABSOLUTE)
    elseif (EXISTS "${Qt6_DIR}/bin")
        get_filename_component(GEANY_BOBUI_RUNTIME_DIR "${Qt6_DIR}/bin" ABSOLUTE)
    endif()
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

if (WIN32 AND GEANY_BOBUI_RUNTIME_DIR)
    file(GENERATE
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/run-geany-bobui-search-studio.bat"
        CONTENT "@echo off\r\nsetlocal\r\nset \"PATH=${GEANY_BOBUI_RUNTIME_DIR};%PATH%\"\r\n\"$<TARGET_FILE:geany-bobui-search-studio>\" %*\r\n")
endif()
