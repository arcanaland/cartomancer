// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include "options.hpp"

#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace cartomancer::cli
{

struct parse_result
{
    options opts;

    // Set when the command line is not valid.
    std::optional<std::string> error;
};

// Parse `args`, which is argv without argv[0].
[[nodiscard]] parse_result parse(std::span<std::string_view const> args);

}  // namespace cartomancer::cli
