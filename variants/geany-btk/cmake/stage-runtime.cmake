if (NOT DEFINED GEANY_BTK_STAGE_ROOT)
    message(FATAL_ERROR "GEANY_BTK_STAGE_ROOT is required")
endif()

if (NOT DEFINED GEANY_BTK_RUNTIME_DIR)
    message(FATAL_ERROR "GEANY_BTK_RUNTIME_DIR is required")
endif()

if (NOT DEFINED GEANY_BTK_TARGET_EXE)
    message(FATAL_ERROR "GEANY_BTK_TARGET_EXE is required")
endif()

string(TIMESTAMP GEANY_BTK_RUNTIME_TOKEN "%Y%m%d-%H%M%S")
set(GEANY_BTK_STAGE_DIR "${GEANY_BTK_STAGE_ROOT}/runtime-bundle-${GEANY_BTK_RUNTIME_TOKEN}")
set(GEANY_BTK_STAGE_MARKER "${GEANY_BTK_STAGE_ROOT}/latest-runtime-bundle.txt")
set(GEANY_BTK_BUNDLE_LAUNCHER "${GEANY_BTK_STAGE_ROOT}/run-geany-btk-bundle.bat")

get_filename_component(GEANY_BTK_TARGET_NAME "${GEANY_BTK_TARGET_EXE}" NAME)

file(REMOVE_RECURSE
    "${GEANY_BTK_STAGE_DIR}/bin"
    "${GEANY_BTK_STAGE_DIR}/platforms"
    "${GEANY_BTK_STAGE_DIR}/imageformats"
    "${GEANY_BTK_STAGE_DIR}/mediaservices"
    "${GEANY_BTK_STAGE_DIR}/playlistformats"
    "${GEANY_BTK_STAGE_DIR}/printerdrivers"
    "${GEANY_BTK_STAGE_DIR}/sqldrivers"
    "${GEANY_BTK_STAGE_DIR}/plugins")
file(MAKE_DIRECTORY "${GEANY_BTK_STAGE_DIR}/bin")

file(COPY "${GEANY_BTK_TARGET_EXE}" DESTINATION "${GEANY_BTK_STAGE_DIR}/bin")

file(GLOB GEANY_BTK_RUNTIME_DLLS "${GEANY_BTK_RUNTIME_DIR}/*.dll")
foreach(runtime_dll IN LISTS GEANY_BTK_RUNTIME_DLLS)
    file(COPY "${runtime_dll}" DESTINATION "${GEANY_BTK_STAGE_DIR}/bin")
endforeach()

if (DEFINED GEANY_BTK_PLUGIN_DIR AND NOT GEANY_BTK_PLUGIN_DIR STREQUAL "" AND EXISTS "${GEANY_BTK_PLUGIN_DIR}")
    file(GLOB GEANY_BTK_PLUGIN_DLLS "${GEANY_BTK_PLUGIN_DIR}/*.dll")
    foreach(plugin_dll IN LISTS GEANY_BTK_PLUGIN_DLLS)
        get_filename_component(plugin_name "${plugin_dll}" NAME)
        set(plugin_destination "${GEANY_BTK_STAGE_DIR}/plugins")

        if (plugin_name MATCHES "^CsGuiWin")
            set(plugin_destination "${GEANY_BTK_STAGE_DIR}/platforms")
        elseif (plugin_name MATCHES "^CsImageFormats")
            set(plugin_destination "${GEANY_BTK_STAGE_DIR}/imageformats")
        elseif (plugin_name MATCHES "^CsMultimedia_DirectShow")
            set(plugin_destination "${GEANY_BTK_STAGE_DIR}/mediaservices")
        elseif (plugin_name MATCHES "^CsMultimedia_m3u")
            set(plugin_destination "${GEANY_BTK_STAGE_DIR}/playlistformats")
        elseif (plugin_name MATCHES "^CsPrinterDriver")
            set(plugin_destination "${GEANY_BTK_STAGE_DIR}/printerdrivers")
        elseif (plugin_name MATCHES "^CsSqlOdbc")
            set(plugin_destination "${GEANY_BTK_STAGE_DIR}/sqldrivers")
        endif()

        file(MAKE_DIRECTORY "${plugin_destination}")
        file(COPY "${plugin_dll}" DESTINATION "${plugin_destination}")
    endforeach()
endif()

file(WRITE "${GEANY_BTK_STAGE_DIR}/run-geany-btk-search-studio.bat"
    "@echo off\r\n"
    "setlocal\r\n"
    "set \"PATH=%~dp0bin;%~dp0platforms;%PATH%\"\r\n"
    "set \"QT_PLUGIN_PATH=%~dp0\"\r\n"
    "set \"QT_QPA_PLATFORM_PLUGIN_PATH=%~dp0platforms\"\r\n"
    "set \"CS_PLUGIN_PATH=%~dp0\"\r\n"
    "\"%~dp0bin\\${GEANY_BTK_TARGET_NAME}\" %*\r\n")

file(WRITE "${GEANY_BTK_STAGE_MARKER}" "${GEANY_BTK_STAGE_DIR}\n")
file(WRITE "${GEANY_BTK_BUNDLE_LAUNCHER}"
    "@echo off\r\n"
    "setlocal\r\n"
    "cd /d \"${GEANY_BTK_STAGE_DIR}\"\r\n"
    "call \"${GEANY_BTK_STAGE_DIR}/run-geany-btk-search-studio.bat\" %*\r\n")

message(STATUS "Staged BTK runtime bundle: ${GEANY_BTK_STAGE_DIR}")
