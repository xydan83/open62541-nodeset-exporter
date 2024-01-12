#
# Author: Vitaly Ogoltsov <vitaly.ogoltsov@me.com>
#

# enable checks with clang-format by default if it is installed on the system
find_program(
        NODESETEXPORTER_CLANG_FORMAT_EXE
        NAMES "clang-format"
)
if (NODESETEXPORTER_CLANG_FORMAT_EXE)
    option(NODESETEXPORTER_CLANG_FORMAT_ENABLE "Enable clang-format checks on source files" ON)
else ()
    option(NODESETEXPORTER_CLANG_FORMAT_ENABLE "Enable clang-format checks on source files" OFF)
endif ()


if (${NODESETEXPORTER_CLANG_FORMAT_ENABLE})
    if (NODESETEXPORTER_CLANG_FORMAT_EXE)
        message(STATUS "ClangFormat: using ${NODESETEXPORTER_CLANG_FORMAT_EXE}")
    else ()
        message(FATAL_ERROR "ClangFormat: unable to find clang-format binary")
    endif ()

    set(CLANG_FORMAT_INCLUDE_PATTERNS ${CLANG_FORMAT_SOURCE_PATTERNS} *.c *.cc *.cpp *.cxx *.h *.hpp *.hxx)
    set(CLANG_FORMAT_EXCLUDE_PATTERNS ${CLANG_FORMAT_EXCLUDE_PATTERNS} ${CMAKE_SOURCE_DIR}/venv)
    set(CLANG_FORMAT_EXCLUDE_PATTERNS ${CLANG_FORMAT_EXCLUDE_PATTERNS} ${CMAKE_SOURCE_DIR}/cmake-) # CMake build directories
    set(CLANG_FORMAT_EXCLUDE_PATTERNS ${CLANG_FORMAT_EXCLUDE_PATTERNS} ${CMAKE_SOURCE_DIR}/build) # CMake build directories
    set(CLANG_FORMAT_EXCLUDE_PATTERNS ${CLANG_FORMAT_EXCLUDE_PATTERNS} ${CMAKE_BINARY_DIR})

    file(GLOB_RECURSE CLANG_FORMAT_SOURCE_FILES ${CLANG_FORMAT_INCLUDE_PATTERNS})
    foreach (SOURCE_FILE ${CLANG_FORMAT_SOURCE_FILES})
        foreach (EXCLUDE_PATTERN ${CLANG_FORMAT_EXCLUDE_PATTERNS})
            string(FIND ${SOURCE_FILE} ${EXCLUDE_PATTERN} EXCLUDE_FOUND)
            if (NOT ${EXCLUDE_FOUND} EQUAL -1)
                list(REMOVE_ITEM CLANG_FORMAT_SOURCE_FILES ${SOURCE_FILE})
            endif ()
        endforeach ()
    endforeach ()

    add_custom_target(
            ClangFormat
            ALL
            COMMAND
            ${NODESETEXPORTER_CLANG_FORMAT_EXE}
            -style=file
            --dry-run
            -Werror
            ${CLANG_FORMAT_SOURCE_FILES}
    )

    add_custom_target(
            ClangFormatFix
            COMMAND
            ${NODESETEXPORTER_CLANG_FORMAT_EXE}
            -style=file
            -i
            -Werror
            ${CLANG_FORMAT_SOURCE_FILES}
    )

else ()
    message(WARNING "ClangFormat: disabled")
endif ()

