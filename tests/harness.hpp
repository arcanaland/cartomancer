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

// Run the CLI in-process against the fixture library.
[[nodiscard]] inline invocation run_cli(
    std::initializer_list<std::string_view> args, cli::theme look = {}
)
{
    std::vector<std::string_view> const argv(args);

    std::ostringstream out;
    std::ostringstream err;

    arcana::library_options options;
    options.roots = {library_root()};

    cli::streams sink{.out = out, .err = err, .style = look};
    int const status = run_with_library(argv, std::move(options), sink);

    return {.status = status, .out = out.str(), .err = err.str()};
}

}  // namespace cartomancer::testing
