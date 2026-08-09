# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

# Under include/ so the generated directory does not collide with the
# `cartomancer` executable, which lands in this same binary dir.
configure_file(
    version.hpp.in "${CMAKE_CURRENT_BINARY_DIR}/include/cartomancer/version.hpp" @ONLY
  )

# Everything except main() lives here so the tests can drive the CLI in-process
# rather than shelling out to a binary.
add_library(
    cartomancer_core STATIC
    app.cpp
    cli.cpp
    color.cpp
    json.cpp
    catalogue.cpp
    list.cpp
    validate.cpp
  )

target_include_directories(
    cartomancer_core
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/include"
  )

target_link_libraries(cartomancer_core PUBLIC arcana::arcana)

add_executable(cartomancer main.cpp)
target_link_libraries(cartomancer PRIVATE cartomancer_core)
