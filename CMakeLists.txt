#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
#

cmake_minimum_required(VERSION 3.18)

project(nodesetexporter
        VERSION 1.1.0
        HOMEPAGE_URL "https://github.com/xydan83/open62541-nodeset-exporter"
        LANGUAGES C CXX)

set(PROJECT_VERSION_SUFFIX "")

include(GNUInstallDirs)

# -------------BUILD OPTIONS-------------
# Set default build type.
option(CMAKE_BUILD_TYPE "Build type " Release)
# Set the option to enable code using a performance timer
option(NODESETEXPORTER_PERFORMANCE_TIMER_ENABLED "Add performance timer code" OFF)
# (optional) Use the Conan package manager to download, build, and install dependencies for specific versions.
option(NODESETEXPORTER_CONAN_ENABLE "Use Conan package manager for resolving project dependencies" ON)
# This option allows you to disable the build of tests from the default build.
option(NODESETEXPORTER_BUILD_TESTS "Include tests in build" OFF)
# This option allows you to create a command utility that can export a set of nodes from the OPC UA Server node space.
option(NODESETEXPORTER_CLI_ENABLE "Add nodesetexporter command utility to build" OFF)
# If you want to add the open62541 library as a static submodule.
option(NODESETEXPORTER_OPEN62541_IS_SUBMODULE "Switch ON this if Open62541 is included as a submodule (built-in) as a static library" OFF)
# If present and true, this will cause all libraries to be built shared unless the library was explicitly added as a static library.
option(BUILD_SHARED_LIBS "Build shared library" OFF)

set(OPEN62541_VERSIONS "v1.3.x" "v1.4.x")
set(OPEN62541_VERSION "v1.3.x" CACHE STRING "What version of the Open62541 library are you using?")
SET_PROPERTY(CACHE OPEN62541_VERSION PROPERTY STRINGS "v1.3.x" ${OPEN62541_VERSIONS})
if ("${OPEN62541_VERSION}" STREQUAL "v1.4.x")
    add_compile_definitions(OPEN62541_VER_1_4)
    add_compile_definitions(OPEN62541_UAPRINT_WITH_QUOTES)
else ()
    add_compile_definitions(OPEN62541_VER_1_3)
endif ()
set(TEST_SERVER_NODESET_DIR "server_nodeset")
# ---------------------------------------

set(BUILD_TESTING ${NODESETEXPORTER_BUILD_TESTS})

# Installing the C++ Language Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set compiler flags.
set(CMAKE_CXX_FLAGS_DEBUG_INIT "-Wall -Werror")
set(CMAKE_CXX_FLAGS_RELEASE_INIT "-Wall -Werror")

# Set default build type.
if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "CMAKE_BUILD_TYPE is not set. Set it to 'Release' by default.")
    set(CMAKE_BUILD_TYPE "Release")
endif ()

# Enable link-time optimizations for the release build.
if (CMAKE_BUILD_TYPE MATCHES "^(Release|RelWithDebInfo|MinSizeRel)$")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif ()

# Enable CMake to save executed commands to a file.
# For more details see: http://clang.llvm.org/docs/HowToSetupToolingForLLVM.html
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

configure_file(
        include/nodesetexporter/Build.h.in
        include/nodesetexporter/Build.h
        @ONLY
)

# Connect Clang extensions.
include(.cmake/ClangFormat.cmake)
include(.cmake/ClangTidy.cmake)

if (${NODESETEXPORTER_CONAN_ENABLE})
    include(.cmake/Conan.cmake)

    # Using the build directory for the Conan cache, not considering others.
    if (NOT DEFINED ENV{CONAN_USER_HOME})
        set(ENV{CONAN_USER_HOME} ${CMAKE_BINARY_DIR})
    endif ()

    # Check out the Conan version
    conan_check(
            REQUIRED
            VERSION 1.61.0
    )

    # Install Conan configuration (repositories, profiles, etc.).
    conan_config_install(ITEM ${CMAKE_SOURCE_DIR}/.conan/config)
    # Install dependencies using Conan.
    conan_cmake_run(
            CONANFILE .conan/conanfile.txt
            BASIC_SETUP CMAKE_TARGETS
            BUILD missing
            GENERATORS cmake_find_package
            ENV CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER}
    )
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup()

    set(CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR} ${CMAKE_MODULE_PATH})
    set(CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR} ${CMAKE_PREFIX_PATH})
    #
endif ()

# Connect the Boost library
set(Boost_USE_STATIC_LIBS ON)
find_package(
        Boost
        1.79.0
        REQUIRED
        COMPONENTS
        program_options
)
add_definitions(-DBOOST_ALLOW_DEPRECATED_HEADERS)
# Connect Python3
find_package(Python3 REQUIRED)
set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE})
# Connect the FMT lightweight text formatting library
find_package(fmt)
if (fmt_FOUND)
    message(STATUS "fmt package has been found.")
    add_compile_definitions(USING_FMT)
endif ()
# Connect OPC-UA Open62541 as a shared library
if (NOT ${NODESETEXPORTER_OPEN62541_IS_SUBMODULE})
    find_package(open62541)
endif ()

# Since in the Open62541 library, from some version (or commit), the UA_print function produces the result in quotes,
# when the definition is activated, the quotes will be removed. (necessary for compatibility).
# add_compile_definitions(OPEN62541_UAPRINT_WITH_QUOTES)

if (NODESETEXPORTER_PERFORMANCE_TIMER_ENABLED)
    message("Performance timer code has been added to nodesetexporter.")
    add_compile_definitions(PERFORMANCE_TIMER_ENABLED)
endif ()

# Generating associative containers of name aliases to TypeNodeID (DataType, ReferenceType)
set(UA_FILE_NODEIDS ${PROJECT_SOURCE_DIR}/toolchain/NodeIds.csv)
set(UA_FILE_NODEIDS_GEN_UTIL ${PROJECT_SOURCE_DIR}/toolchain/aliases_map_maker.py)
set(UA_STATUS_CODE_DATATYPE_HEADER_DIR ${CMAKE_BINARY_DIR}/include/nodesetexporter/common)
set(UA_STATUS_CODE_DATATYPE_HEADER_FILENAME DatatypeAliases.h)

# Loading NodeIds.csv from a opcfoundation.org resource
if (NOT EXISTS ${UA_FILE_NODEIDS})
    file(DOWNLOAD https://raw.githubusercontent.com/OPCFoundation/UA-Nodeset/v1.04/Schema/NodeIds.csv ${UA_FILE_NODEIDS} SHOW_PROGRESS)
endif ()

# Header file generation
if (NOT EXISTS "${UA_FILE_NODEIDS}")
    message(FATAL_ERROR "File not found ${UA_FILE_NODEIDS}")
endif ()

if (NOT EXISTS "${UA_FILE_NODEIDS_GEN_UTIL}")
    message(FATAL_ERROR "A utility for generating a name alias file for TypeNodeID was not found (DataType, ReferenceType): ${UA_FILE_NODEIDS_GEN_UTIL}")
endif ()

file(MAKE_DIRECTORY ${UA_STATUS_CODE_DATATYPE_HEADER_DIR})
execute_process(
        COMMAND ${PYTHON_EXECUTABLE} ${UA_FILE_NODEIDS_GEN_UTIL} --nodeids_path ${UA_FILE_NODEIDS} --path_to_header ${UA_STATUS_CODE_DATATYPE_HEADER_DIR}/${UA_STATUS_CODE_DATATYPE_HEADER_FILENAME}
        TIMEOUT 60
        ERROR_VARIABLE CMD_ERROR)
if (${CMD_ERROR})
    message(FATAL_ERROR "The status code file could not be generated. Script error code: ${CMD_ERROR}")
endif ()

# Forming the nodesetexporter library
set(NODESETEXPORTER_INTERNAL_PUBLIC_HEADERS
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/nodesetexporter/interfaces/IEncoder.h>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/nodesetexporter/interfaces/IOpen62541.h>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/nodesetexporter/encoders/GetAttributeToXMLText.h>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/nodesetexporter/encoders/XMLEncoder.h>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/nodesetexporter/open62541/UATypesContainer.h>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/nodesetexporter/open62541/ClientWrappers.h>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/nodesetexporter/open62541/ServerWrappers.h>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/nodesetexporter/open62541/TypeAliases.h>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/nodesetexporter/open62541/NodeIntermediateModel.h>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/nodesetexporter/open62541/BrowseOperations.h>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/nodesetexporter/logger/StdLog.h>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/nodesetexporter/logger/LogPlugin.h>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/nodesetexporter/NodesetExporter.h>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/nodesetexporter/NodesetExporterLoop.h>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/nodesetexporter/common/LoggerBase.h>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/nodesetexporter/common/Statuses.h>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/nodesetexporter/common/Encoder_types.h>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/nodesetexporter/common/Strings.h>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/nodesetexporter/common/PerformanceTimer.h>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/nodesetexporter/common/Open62541CompatibilityCheck.h>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include/nodesetexporter/common/DatatypeAliases.h>
        CACHE INTERNAL "")

set(NODESETEXPORTER_INTERNAL_PRIVATE_SOURCE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/nodesetexporter/open62541/TypeAliases.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/nodesetexporter/open62541/BrowseOperations.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/nodesetexporter/open62541/ClientWrappers.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/nodesetexporter/NodesetExporterLoop.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/nodesetexporter/NodesetExporter.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/nodesetexporter/logger/LogPlugin.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/nodesetexporter/common/Strings.cpp
        CACHE INTERNAL "")

# Forming the nodesetexporter library for cli utility and for tests
if (${NODESETEXPORTER_CLI_ENABLE} OR ${NODESETEXPORTER_BUILD_TESTS})
    add_library(${PROJECT_NAME}-for-cli STATIC)
    target_sources(
            ${PROJECT_NAME}-for-cli
            PUBLIC
            ${NODESETEXPORTER_INTERNAL_PUBLIC_HEADERS}
            PRIVATE
            ${NODESETEXPORTER_INTERNAL_PRIVATE_SOURCE}

    )

    target_include_directories(
            ${PROJECT_NAME}-for-cli
            PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/nodesetexporter/>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/nodesetexporter/common/>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/nodesetexporter/open62541/>
            $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include/>
    )

    target_link_libraries(
            ${PROJECT_NAME}-for-cli
            PRIVATE
            open62541::open62541
            tinyxml2
            fmt::fmt
    )

    nodesetexporter_clang_format_setup(${PROJECT_NAME}-for-cli)
endif ()

add_library(${PROJECT_NAME})
target_sources(
        ${PROJECT_NAME}
        PUBLIC
        ${NODESETEXPORTER_INTERNAL_PUBLIC_HEADERS}
        PRIVATE
        ${NODESETEXPORTER_INTERNAL_PRIVATE_SOURCE}
)

target_include_directories(
        ${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/nodesetexporter/>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/nodesetexporter/common/>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/nodesetexporter/open62541/>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include/>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE
        open62541::open62541
        tinyxml2
        fmt::fmt
)

# Setting up the test part of the project
# Downloading UANodeSet.xsd from opcfoundation.org
if (NOT EXISTS ${PROJECT_SOURCE_DIR}/test/nodesetexporter/server_nodeset/UANodeSet.xsd)
    file(DOWNLOAD https://raw.githubusercontent.com/OPCFoundation/UA-Nodeset/v1.04/Schema/UANodeSet.xsd ${PROJECT_SOURCE_DIR}/test/nodesetexporter/server_nodeset/UANodeSet.xsd SHOW_PROGRESS)
endif ()

# Forming the tests
if (${NODESETEXPORTER_BUILD_TESTS})

    include(CTest)
    enable_testing()
    include(.cmake/Tests.cmake)
    # Connect a library of unit tests
    find_package(doctest)
    # Connect a library of mocks for testing
    find_package(trompeloeil)

    # Finding a Dynamic Library for the xml nodeset test
    find_library(XMLPLUSPLUS NAMES xml++-5.0)
    message("libxml++ path: " ${XMLPLUSPLUS})
    SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_SOURCE_DIR}/test/nodesetexporter/${TEST_SERVER_NODESET_DIR}/ex_nodeset.c PROPERTIES COMPILE_FLAGS -O0)

    add_executable(
            ${PROJECT_NAME}-tests
            ${CMAKE_CURRENT_SOURCE_DIR}/test/nodesetexporter/open62541/UATypesContainerTest.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/test/nodesetexporter/open62541/NodeIntermediateModelTest.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/test/nodesetexporter/open62541/TypeAliasesTest.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/test/nodesetexporter/open62541/StdLogTest.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/test/nodesetexporter/open62541/ClientWrappersTest.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/test/nodesetexporter/open62541/BrowseOperationsTest.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/test/nodesetexporter/encoders/XMLEncoderTest.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/test/nodesetexporter/common/PerformanceTimerTest.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/test/nodesetexporter/NodesetExporterLoopTest.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/test/nodesetexporter/NodesetExporterTest.cpp
    )

    target_sources(
            ${PROJECT_NAME}-tests
            PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/test/nodesetexporter/${TEST_SERVER_NODESET_DIR}/ex_nodeset.h
            ${CMAKE_CURRENT_SOURCE_DIR}/test/nodesetexporter/common_for_tests/XmlHelperFunctions.h
            ${CMAKE_CURRENT_SOURCE_DIR}/include/nodesetexporter/open62541/UATypesContainer.h
            ${CMAKE_CURRENT_SOURCE_DIR}/include/nodesetexporter/common/DateTime.h
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/test/nodesetexporter/${TEST_SERVER_NODESET_DIR}/ex_nodeset.c
            ${CMAKE_CURRENT_SOURCE_DIR}/src/nodesetexporter/common/DateTime.cpp
    )

    target_include_directories(
            ${PROJECT_NAME}-tests
            PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/test/nodesetexporter/server_nodeset
            ${CMAKE_CURRENT_SOURCE_DIR}/test/nodesetexporter/common_for_tests
            ${CMAKE_BINARY_DIR}/include/
    )

    target_link_libraries(
            ${PROJECT_NAME}-tests
            PRIVATE
            ${PROJECT_NAME}-for-cli
            lib-testing
            doctest::doctest
            trompeloeil::trompeloeil
            tinyxml2
            ${XMLPLUSPLUS})

    if (fmt_FOUND)
        target_link_libraries(
                ${PROJECT_NAME}-tests
                PRIVATE
                fmt::fmt)
    endif ()

    # Copying UANodeSet.xsd for encoder test
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    configure_file(test/nodesetexporter/server_nodeset/UANodeSet.xsd ${CMAKE_BINARY_DIR}/bin COPYONLY)
    configure_file(test/nodesetexporter/server_nodeset/UANodeSet.xsd ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

    add_unit_test(NAME ${PROJECT_NAME}-tests)
    nodesetexporter_clang_format_setup(${PROJECT_NAME}-tests)
endif ()

# Connecting the assembly of the executable export utility.
if (${NODESETEXPORTER_CLI_ENABLE})
    add_subdirectory(apps)
endif ()
# Connecting auxiliary libraries
add_subdirectory(lib)

# Install nodesetexporter library
set(NODESETEXPORTER_EXPORT_PUBLIC_HEADERS
        ${PROJECT_SOURCE_DIR}/include/nodesetexporter/NodesetExporter.h
        ${PROJECT_SOURCE_DIR}/include/nodesetexporter/common/Encoder_types.h
        ${PROJECT_SOURCE_DIR}/include/nodesetexporter/common/LoggerBase.h
        ${PROJECT_SOURCE_DIR}/include/nodesetexporter/open62541/UATypesContainer.h
        ${PROJECT_SOURCE_DIR}/include/nodesetexporter/common/Statuses.h
        CACHE INTERNAL "")

set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${NODESETEXPORTER_EXPORT_PUBLIC_HEADERS}")

install(TARGETS ${PROJECT_NAME}
        EXPORT NodesetExporter
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        PUBLIC_HEADER DESTINATION include/NodesetExporter)

install(FILES nodesetexporter-config.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/NodesetExporter)

install(EXPORT NodesetExporter DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/NodesetExporter)

nodesetexporter_clang_format_setup(${PROJECT_NAME})