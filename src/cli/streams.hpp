// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <iosfwd>

namespace cartomancer::cli
{

// Where a command writes (abstracted for tests).
struct streams
{
    std::ostream& out;
    std::ostream& err;

    // Whether to emit ANSI color on out.
    bool use_color = false;
};

}  // namespace cartomancer::cli
