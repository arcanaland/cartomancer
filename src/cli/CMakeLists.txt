# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

# The command-line surface on its own: argument parsing, the option struct, the
# help text, and the terminal colour decision. Knows nothing about decks, so it
# can be tested without touching the filesystem.
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
            text.hpp
    PRIVATE
        color.cpp
        parse.cpp
        text.cpp
)

target_compile_features(cartomancer-cli PUBLIC cxx_std_26)

# arcana::severity appears in `options` and in the colour and name lookups.
target_link_libraries(cartomancer-cli PUBLIC arcana::arcana)
