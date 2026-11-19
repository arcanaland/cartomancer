# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 4.3)
project(
    cartomancer
    VERSION 0.3.0
    LANGUAGES CXX
    DESCRIPTION "Command line tool for Arcana Land's Tarot decks"
    HOMEPAGE_URL "https://github.com/arcanaland/cartomancer"
    SPDX_LICENSE "MIT"
  )

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

option(CARTOMANCER_BUILD_TESTS "Build the cartomancer test suite" ${PROJECT_IS_TOP_LEVEL})

option(CARTOMANCER_ARCANA_UNPINNED "Build against any arcana (co-development only)" OFF)

file(STRINGS "${PROJECT_SOURCE_DIR}/ARCANA_VERSION" CARTOMANCER_ARCANA_VERSION LIMIT_COUNT 1)
if(NOT CARTOMANCER_ARCANA_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR "ARCANA_VERSION must hold one N.N.N version, got '${CARTOMANCER_ARCANA_VERSION}'")
endif()

if(CARTOMANCER_ARCANA_UNPINNED)
    message(WARNING "Building against an unpinned arcana; ARCANA_VERSION (${CARTOMANCER_ARCANA_VERSION}) is ignored")
    find_package(arcana CONFIG REQUIRED)
else()
    find_package(arcana ${CARTOMANCER_ARCANA_VERSION} EXACT CONFIG REQUIRED)
endif()
find_package(nlohmann_json CONFIG REQUIRED)

add_subdirectory(src)

if(CARTOMANCER_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
