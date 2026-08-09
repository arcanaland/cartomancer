// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <arcana/library.hpp>

#include <iosfwd>
#include <span>
#include <string_view>

namespace cartomancer
{

// Where a command writes. Held by reference so tests can capture into
// std::ostringstream instead of shelling out to the built binary.
struct streams
{
    std::ostream& out;
    std::ostream& err;

    // Whether `out` takes ANSI colour. Resolved once in `run` from --color,
    // NO_COLOR and isatty, so no command has to ask again.
    bool colored = false;
};

// Parse `args` (argv without argv[0]) and run the requested command.
//
// @return the process exit code; see `exit_code` in cli.hpp.
int run(std::span<std::string_view const> args, streams sink);

// As `run`, but with the deck library configured explicitly rather than from
// the standard XDG roots. This is the seam the fixture-rooted tests use.
int run_with_library(
    std::span<std::string_view const> args, arcana::library_options lib_options, streams sink
);

}  // namespace cartomancer
