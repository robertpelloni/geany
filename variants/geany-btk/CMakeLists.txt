cmake_minimum_required(VERSION 3.16)
project(geany_btk_variant LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Experimental alternate frontend.
# This is intentionally isolated from the current production build.
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../subprojects/btk ${CMAKE_CURRENT_BINARY_DIR}/btk EXCLUDE_FROM_ALL)

add_executable(geany-btk-search-studio
    src/main.cpp
)

target_link_libraries(geany-btk-search-studio
    PRIVATE
        BTK::Core
        BTK::Gui
)

target_compile_definitions(geany-btk-search-studio PRIVATE GEANY_BTK_EXPERIMENTAL=1)
