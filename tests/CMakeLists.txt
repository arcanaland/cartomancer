# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

find_package(Catch2 3 REQUIRED)
include(Catch)

add_executable(cartomancer_test)

target_sources(
    cartomancer_test
    PRIVATE
        harness.hpp
        catalogue_test.cpp
        cli_test.cpp
        color_test.cpp
        json_test.cpp
        list_test.cpp
        validate_test.cpp
)

target_compile_features(cartomancer_test PRIVATE cxx_std_26)

target_link_libraries(
    cartomancer_test PRIVATE cartomancer::core Catch2::Catch2WithMain
  )

target_compile_definitions(
    cartomancer_test
    PRIVATE CARTOMANCER_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
  )

catch_discover_tests(cartomancer_test)
