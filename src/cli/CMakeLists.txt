# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

add_library(cartomancer-cli STATIC)
add_library(cartomancer::cli ALIAS cartomancer-cli)

target_sources(
    cartomancer-cli
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${PROJECT_SOURCE_DIR}/src
        FILES
            color.hpp
            options.hpp
            parse.hpp
            streams.hpp
            surface.hpp
            text.hpp
    PRIVATE
        color.cpp
        parse.cpp
        surface.cpp
        text.cpp
)

target_compile_features(cartomancer-cli PUBLIC cxx_std_26)

target_link_libraries(cartomancer-cli PUBLIC arcana::arcana)
