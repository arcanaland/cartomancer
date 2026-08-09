// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <iosfwd>

namespace cartomancer::cli
{

// Where a command writes. Passed by value so the tests can substitute
// stringstreams for the process streams.
struct streams
{
    std::ostream& out;
    std::ostream& err;

    // Whether to emit ANSI colour on out. Already resolved from --color,
    // NO_COLOR, and whether out is a terminal -- see resolve_color.
    bool use_color = false;
};

}  // namespace cartomancer::cli
