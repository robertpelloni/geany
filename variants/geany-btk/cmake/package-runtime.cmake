if (NOT DEFINED GEANY_BTK_STAGE_ROOT)
    message(FATAL_ERROR "GEANY_BTK_STAGE_ROOT is required")
endif()

set(GEANY_BTK_STAGE_MARKER "${GEANY_BTK_STAGE_ROOT}/latest-runtime-bundle.txt")

if (NOT EXISTS "${GEANY_BTK_STAGE_MARKER}")
    message(FATAL_ERROR "No staged BTK runtime metadata found at ${GEANY_BTK_STAGE_MARKER}")
endif()

file(READ "${GEANY_BTK_STAGE_MARKER}" GEANY_BTK_STAGE_DIR)
string(STRIP "${GEANY_BTK_STAGE_DIR}" GEANY_BTK_STAGE_DIR)

if (GEANY_BTK_STAGE_DIR STREQUAL "")
    message(FATAL_ERROR "Latest BTK runtime bundle path is empty")
endif()

if (NOT EXISTS "${GEANY_BTK_STAGE_DIR}")
    message(FATAL_ERROR "Latest BTK runtime bundle does not exist: ${GEANY_BTK_STAGE_DIR}")
endif()

get_filename_component(GEANY_BTK_STAGE_NAME "${GEANY_BTK_STAGE_DIR}" NAME)
string(REPLACE "runtime-bundle-" "" GEANY_BTK_RUNTIME_TOKEN "${GEANY_BTK_STAGE_NAME}")
set(GEANY_BTK_RUNTIME_PACKAGE "${GEANY_BTK_STAGE_ROOT}/geany-btk-search-studio-runtime-${GEANY_BTK_RUNTIME_TOKEN}.zip")

file(REMOVE_RECURSE "${GEANY_BTK_RUNTIME_PACKAGE}")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E tar "cf" "${GEANY_BTK_RUNTIME_PACKAGE}" --format=zip .
    WORKING_DIRECTORY "${GEANY_BTK_STAGE_DIR}"
    COMMAND_ERROR_IS_FATAL ANY)

message(STATUS "Packaged BTK runtime bundle: ${GEANY_BTK_RUNTIME_PACKAGE}")
