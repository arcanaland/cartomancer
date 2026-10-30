// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include "app.hpp"

#include <arcana/library.hpp>

#include <filesystem>
#include <initializer_list>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace cartomancer::testing
{

// The fixture tree as an absolute path baked in by CMake.
// Hardcoding so the tests do not depend on the working dir.
[[nodiscard]] inline std::filesystem::path fixtures()
{
    return std::filesystem::path{CARTOMANCER_FIXTURES};
}

[[nodiscard]] inline std::filesystem::path library_root()
{
    return fixtures() / "library";
}

// The root holding the one deck that ships artwork. It is kept out of
// library/ so that adding it did not move every count the list tests assert.
[[nodiscard]] inline std::filesystem::path art_root()
{
    return fixtures() / "art";
}

[[nodiscard]] inline std::filesystem::path art_deck()
{
    return art_root() / "art-deck";
}

// One CLI invocation
struct invocation
{
    int status = 0;
    std::string out;
    std::string err;
};

// A theme at a depth
[[nodiscard]] inline cli::theme styled(cli::color_depth depth)
{
    return {.depth = depth, .style = cli::for_depth(depth)};
}

// Run the CLI in-process against a library.
[[nodiscard]] inline invocation run_cli_with(
    arcana::library_options options, std::initializer_list<std::string_view> args, cli::theme look
)
{
    std::vector<std::string_view> const argv(args);

    std::ostringstream out;
    std::ostringstream err;

    cli::streams sink{.out = out, .err = err, .style = look};
    int const status = run_with_library(argv, std::move(options), sink);

    return {.status = status, .out = out.str(), .err = err.str()};
}

// Run the CLI in-process against the fixture library.
[[nodiscard]] inline invocation run_cli(
    std::initializer_list<std::string_view> args, cli::theme look = {}
)
{
    arcana::library_options options;
    options.roots = {library_root()};

    return run_cli_with(std::move(options), args, look);
}

// Run the CLI against the art root, with art-deck as the reference deck so
// that `show` and `draw` resolve a deck without being told one.
[[nodiscard]] inline invocation run_art_cli(
    std::initializer_list<std::string_view> args, cli::theme look = {}
)
{
    arcana::library_options options;
    options.roots = {art_root()};
    options.reference_deck = art_deck();

    return run_cli_with(std::move(options), args, look);
}

}  // namespace cartomancer::testing
