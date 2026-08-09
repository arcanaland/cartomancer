# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

# Under generated/include/ so the generated directory does not collide with the
# `cartomancer` executable, which lands in this same binary dir.
configure_file(version.hpp.in
               ${PROJECT_BINARY_DIR}/generated/include/cartomancer/version.hpp @ONLY)

add_subdirectory(cli)

# Everything except main() lives here so the tests can drive the CLI in-process
# rather than shelling out to a binary.
add_library(cartomancer-core STATIC)
add_library(cartomancer::core ALIAS cartomancer-core)

target_sources(
    cartomancer-core
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${PROJECT_BINARY_DIR}/generated/include
        FILES
            ${PROJECT_BINARY_DIR}/generated/include/cartomancer/version.hpp
            app.hpp
            catalogue.hpp
            json.hpp
            list.hpp
            validate.hpp
    PRIVATE
        app.cpp
        catalogue.cpp
        json.cpp
        list.cpp
        validate.cpp
)

target_compile_features(cartomancer-core PUBLIC cxx_std_26)

# nlohmann is PUBLIC because json.hpp includes it and the tests include
# json.hpp. cartomancer-core is never installed, so nothing leaks past here.
target_link_libraries(
    cartomancer-core
    PUBLIC arcana::arcana cartomancer::cli nlohmann_json::nlohmann_json
  )

add_executable(cartomancer main.cpp)
add_executable(cartomancer::cartomancer ALIAS cartomancer)

target_link_libraries(cartomancer PRIVATE cartomancer::core)
