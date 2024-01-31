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

    add_custom_target(ClangFormat ALL)
else ()
    message(WARNING "ClangFormat: disabled")
endif ()


function(nodesetexporter_clang_format_setup target)
    if (${NODESETEXPORTER_CLANG_FORMAT_ENABLE})
        get_target_property(target_type ${target} TYPE)
        if (${target_type} STREQUAL "INTERFACE_LIBRARY")
            get_target_property(target_sources ${target} INTERFACE_SOURCES)
        else ()
            get_target_property(target_sources ${target} SOURCES)
        endif ()

        list(FILTER target_sources INCLUDE REGEX "\\.(c|cc|cpp|cxx|h|hpp|hxx)$")

        # Часть файлов исходных кодов является авто-генерируемой, поэтому требуется использовать полные пути до файлов.
        set(target_source_paths)
        foreach (target_source ${target_sources})
            if (EXISTS "${target_source}")
                list(APPEND target_source_paths "${target_source}")
            elseif (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${target_source}")
                list(APPEND target_source_paths "${CMAKE_CURRENT_SOURCE_DIR}/${target_source}")
            else ()
                list(APPEND target_source_paths "${CMAKE_CURRENT_BINARY_DIR}/${target_source}")
            endif ()
        endforeach ()

        if (target_sources)
            add_custom_target(
                    ${target}-clang-format
                    COMMAND
                    ${NODESETEXPORTER_CLANG_FORMAT_EXE}
                    -style=file
                    --dry-run
                    -Werror
                    ${target_source_paths}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    COMMENT "Analyzing ${target} sources with clang-format..."
            )
            add_dependencies(ClangFormat ${target}-clang-format)
        else ()
            message(STATUS "ClangFormat: no source found for ${target}")
        endif ()
    endif ()
endfunction()
