cmake_minimum_required(VERSION 3.16)
project(geany_btk_variant LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Experimental alternate frontend.
# This is intentionally isolated from the current production build.
# BTK is currently consumed as a standalone CMake package/build tree instead of
# as an add_subdirectory() dependency because the BTK project still assumes it
# is configured from its own source root.
set(GEANY_BTK_HINT_PATHS
    "${CMAKE_CURRENT_SOURCE_DIR}/../../build/btk-install/lib/cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../build/btk-install/cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../build/btk-ninja-fixes"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../build/btk-ninja"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../build/btk"
)

find_package(BTK CONFIG QUIET PATHS ${GEANY_BTK_HINT_PATHS})
if (NOT BTK_FOUND)
    find_package(BTK CONFIG REQUIRED)
endif()

set(GEANY_BTK_RUNTIME_DIR "")
if (DEFINED BTK_DIR)
    if (EXISTS "${BTK_DIR}/../../../bin")
        get_filename_component(GEANY_BTK_RUNTIME_DIR "${BTK_DIR}/../../../bin" ABSOLUTE)
    elseif (EXISTS "${BTK_DIR}/../../bin")
        get_filename_component(GEANY_BTK_RUNTIME_DIR "${BTK_DIR}/../../bin" ABSOLUTE)
    elseif (EXISTS "${BTK_DIR}/bin")
        get_filename_component(GEANY_BTK_RUNTIME_DIR "${BTK_DIR}/bin" ABSOLUTE)
    endif()
endif()

add_executable(geany-btk-search-studio
    src/main.cpp
)

target_link_libraries(geany-btk-search-studio
    PRIVATE
        BTK::Core
        BTK::Gui
)

target_compile_definitions(geany-btk-search-studio PRIVATE GEANY_BTK_EXPERIMENTAL=1)

if (WIN32 AND GEANY_BTK_RUNTIME_DIR)
    file(GENERATE
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/run-geany-btk-search-studio.bat"
        CONTENT "@echo off\r\nsetlocal\r\nset \"PATH=${GEANY_BTK_RUNTIME_DIR};%PATH%\"\r\n\"$<TARGET_FILE:geany-btk-search-studio>\" %*\r\n")
endif()
