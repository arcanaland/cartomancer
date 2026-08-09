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

// The fixture tree, as an absolute path baked in by CMake so the tests do not
// depend on the working directory ctest happens to use.
[[nodiscard]] inline std::filesystem::path fixtures()
{
    return std::filesystem::path{CARTOMANCER_FIXTURES};
}

// The fixture deck library root. Pointing library_options::roots here is what
// keeps these tests off whatever decks are installed on the build machine.
[[nodiscard]] inline std::filesystem::path library_root()
{
    return fixtures() / "library";
}

// One CLI invocation and everything it produced.
struct invocation
{
    int status = 0;
    std::string out;
    std::string err;
};

// Run the CLI in-process against the fixture library.
[[nodiscard]] inline invocation run_cli(std::initializer_list<std::string_view> args)
{
    std::vector<std::string_view> const argv(args);

    std::ostringstream out;
    std::ostringstream err;

    arcana::library_options options;
    options.roots = {library_root()};

    cli::streams sink{.out = out, .err = err, .use_color = false};
    int const status = run_with_library(argv, std::move(options), sink);

    return {.status = status, .out = out.str(), .err = err.str()};
}

// As run_cli, but with colour forced on so the SGR sequences can be asserted.
[[nodiscard]] inline invocation run_cli_with_color(std::initializer_list<std::string_view> args)
{
    std::vector<std::string_view> const argv(args);

    std::ostringstream out;
    std::ostringstream err;

    arcana::library_options options;
    options.roots = {library_root()};

    cli::streams sink{.out = out, .err = err, .use_color = true};
    int const status = run_with_library(argv, std::move(options), sink);

    return {.status = status, .out = out.str(), .err = err.str()};
}

}  // namespace cartomancer::testing
