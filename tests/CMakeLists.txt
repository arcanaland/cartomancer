# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

find_package(Catch2 3 REQUIRED)

add_executable(
    cartomancer_test
    cli_test.cpp
    color_test.cpp
    json_test.cpp
    catalogue_test.cpp
    list_test.cpp
    validate_test.cpp
  )

target_link_libraries(cartomancer_test PRIVATE cartomancer_core Catch2::Catch2WithMain)

target_compile_definitions(
    cartomancer_test PRIVATE "CARTOMANCER_FIXTURES=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures\""
  )

include(Catch)
catch_discover_tests(cartomancer_test)
