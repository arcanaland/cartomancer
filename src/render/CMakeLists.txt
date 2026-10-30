# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

add_library(cartomancer-render STATIC)
add_library(cartomancer::render ALIAS cartomancer-render)

target_sources(
    cartomancer-render
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${PROJECT_SOURCE_DIR}/src
        FILES
            art.hpp
            escape.hpp
            image.hpp
            layout.hpp
            quantize.hpp
    PRIVATE
        art.cpp
        escape.cpp
        image.cpp
        layout.cpp
        quantize.cpp
)

target_compile_features(cartomancer-render PUBLIC cxx_std_26)

target_link_libraries(
    cartomancer-render
    PUBLIC
        arcana::arcana
        cartomancer::cli
    PRIVATE
        stb::stb
  )
