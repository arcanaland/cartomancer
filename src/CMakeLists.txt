# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

configure_file(version.hpp.in
               ${PROJECT_BINARY_DIR}/generated/include/cartomancer/version.hpp @ONLY)

add_subdirectory(cli)
add_subdirectory(render)

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
            draw.hpp
            json.hpp
            list.hpp
            show.hpp
            target.hpp
            validate.hpp
    PRIVATE
        app.cpp
        catalogue.cpp
        draw.cpp
        json.cpp
        list.cpp
        show.cpp
        target.cpp
        validate.cpp
)

target_compile_features(cartomancer-core PUBLIC cxx_std_26)

target_link_libraries(
    cartomancer-core
    PUBLIC
        arcana::arcana
        cartomancer::cli
        cartomancer::render
        nlohmann_json::nlohmann_json
  )

add_executable(cartomancer main.cpp)
add_executable(cartomancer::cartomancer ALIAS cartomancer)

target_link_libraries(cartomancer PRIVATE cartomancer::core)
