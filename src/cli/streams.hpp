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
    theme style;
};

}  // namespace cartomancer::cli
