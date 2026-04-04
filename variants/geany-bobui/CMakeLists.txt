cmake_minimum_required(VERSION 3.16)
project(geany_bobui_variant LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Experimental alternate frontend.
# This is intentionally isolated from the GTK-based production build.
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../subprojects/bobui ${CMAKE_CURRENT_BINARY_DIR}/bobui EXCLUDE_FROM_ALL)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

add_executable(geany-bobui-search-studio
    src/main.cpp
)

target_link_libraries(geany-bobui-search-studio
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
)

target_compile_definitions(geany-bobui-search-studio PRIVATE GEANY_BOBUI_EXPERIMENTAL=1)
