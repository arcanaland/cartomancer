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

    // Whether out takes ANSI colour.
    bool colored = false;
};

}  // namespace cartomancer::cli
