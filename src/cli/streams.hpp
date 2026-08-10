// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include "color.hpp"

#include <iosfwd>

namespace cartomancer::cli
{

// Where a command writes (abstracted for tests).
struct streams
{
    std::ostream& out;
    std::ostream& err;

    // How to dress what goes on them. `style.style` is the palette; the default
    // is no colour, ASCII marks and unbounded width.
    theme style;
};

}  // namespace cartomancer::cli
