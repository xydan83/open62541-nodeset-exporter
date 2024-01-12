#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
#
# Entry point for doctest
#

project(lib-testing VERSION ${CMAKE_PROJECT_VERSION} LANGUAGES CXX)

add_library(${PROJECT_NAME} STATIC)
target_sources(
        ${PROJECT_NAME}
        PRIVATE
        src/main.cpp
)
target_include_directories(
        ${PROJECT_NAME}
        PUBLIC
        include/
)
target_link_libraries(
        ${PROJECT_NAME}
        PUBLIC
        doctest::doctest
)


add_executable(
        ${PROJECT_NAME}-tests
        test/MainTest.cpp
)
target_link_libraries(
        ${PROJECT_NAME}-tests
        ${PROJECT_NAME}
)
add_unit_test(NAME ${PROJECT_NAME}-tests)
