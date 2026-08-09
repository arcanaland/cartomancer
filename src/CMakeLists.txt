# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

configure_file(version.hpp.in
               ${PROJECT_BINARY_DIR}/generated/include/cartomancer/version.hpp @ONLY)

add_subdirectory(cli)

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

target_link_libraries(
    cartomancer-core
    PUBLIC
        arcana::arcana
        cartomancer::cli
        nlohmann_json::nlohmann_json
  )

add_executable(cartomancer main.cpp)
add_executable(cartomancer::cartomancer ALIAS cartomancer)

target_link_libraries(cartomancer PRIVATE cartomancer::core)
