#
# Author: Vitaly Ogoltsov <vitaly.ogoltsov@me.com>
#


# Bash is required to run tests that redirect standard output.
find_program(
        BASH
        NAMES "bash"
)

# Create a directory for test run reports.
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/TestReports)

# Registers a test with CTest, setting the junit output format to allow automatic collection and analysis.
function(add_unit_test)
    set(oneValueArgs NAME)
    cmake_parse_arguments(UnitTest "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (BASH)
        add_test(
                NAME ${UnitTest_NAME}
                COMMAND $<TARGET_FILE:${UnitTest_NAME}> -s --reporters=junit --out=${CMAKE_BINARY_DIR}/TestReports/${UnitTest_NAME}.xml
        )
    else()
        message(FATAL_ERROR "Failed to determine shell to run unit test")
    endif()

    # Optionally, turn off
    if (NOT ${NODESETEXPORTER_BUILD_TESTS})
        set_property(TARGET ${UnitTest_NAME} PROPERTY EXCLUDE_FROM_ALL)
    endif()
endfunction() # add_unit_test
