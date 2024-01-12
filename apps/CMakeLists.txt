#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
#

project(cli_nodesetexporter
        VERSION ${CMAKE_PROJECT_VERSION}
        DESCRIPTION "Connects the OPC-UA client to the Server and downloads the structure from the specified starting node."
        LANGUAGES CXX)

find_package(open62541)

add_library(${PROJECT_NAME}-static STATIC)
target_sources(
        ${PROJECT_NAME}-static
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include/apps/nodesetexporter/Application.h
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/apps/nodesetexporter/Application.cpp
)

target_include_directories(
        ${PROJECT_NAME}-static
        PUBLIC
        ${CMAKE_BINARY_DIR}
        ${CMAKE_SOURCE_DIR}
        include/
)

target_link_libraries(
        ${PROJECT_NAME}-static
        PUBLIC
        nodesetexporter-for-cli
        PRIVATE
        Boost::program_options
        open62541::open62541
)

add_executable(${PROJECT_NAME})
target_sources(
        ${PROJECT_NAME}
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
)

target_link_libraries(
        ${PROJECT_NAME}
        PUBLIC
        ${PROJECT_NAME}-static
)

# Install cli_nodesetexporter utility application
install(TARGETS ${PROJECT_NAME} RUNTIME COMPONENT Runtime)