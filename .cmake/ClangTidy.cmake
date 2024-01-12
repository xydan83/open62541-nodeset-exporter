#
# Author: Vitaly Ogoltsov <vitaly.ogoltsov@me.com>
#

# enable checks with clang-tidy by default if it is installed on the system
find_program(
        NODESETEXPORTER_CLANG_TIDY_EXE
        NAMES "clang-tidy"
)
if (NODESETEXPORTER_CLANG_TIDY_EXE)
    option(NODESETEXPORTER_CLANG_TIDY_ENABLE "Enable clang-tidy checks on source files" ON)
else ()
    option(NODESETEXPORTER_CLANG_TIDY_ENABLE "Enable clang-tidy checks on source files" OFF)
endif ()


if (${NODESETEXPORTER_CLANG_TIDY_ENABLE})
    if (NODESETEXPORTER_CLANG_TIDY_EXE)
        message(STATUS "ClangTidy: using ${NODESETEXPORTER_CLANG_TIDY_EXE}")
    else ()
        message(FATAL_ERROR "ClangTidy: unable to find clang-tidy binary")
    endif ()

    set(
            CMAKE_CXX_CLANG_TIDY
            ${NODESETEXPORTER_CLANG_TIDY_EXE}
            -format-style=file
            -p ${CMAKE_BINARY_DIR}
    )

else ()
    message(WARNING "ClangTidy: disabled")
endif ()

