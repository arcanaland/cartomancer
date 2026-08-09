# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 4.3)
project(
    cartomancer
    VERSION 0.3.0
    LANGUAGES CXX
    SPDX_LICENSE "MIT"
  )

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

option(CARTOMANCER_BUILD_TESTS "Build the cartomancer test suite" ${PROJECT_IS_TOP_LEVEL})

# The identifier is `arcana`; `libarcana` is only the repo and the .a filename.
find_package(arcana CONFIG REQUIRED)

add_subdirectory(src)

if(CARTOMANCER_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
