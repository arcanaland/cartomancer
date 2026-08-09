// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include "options.hpp"

#include <expected>
#include <span>
#include <string>
#include <string_view>

namespace cartomancer::cli
{

// A whole command line a message explaining why it is not one.
using parse_result = std::expected<options, std::string>;

// @param args argv without argv[0].
[[nodiscard]] parse_result parse(std::span<std::string_view const> args);

}  // namespace cartomancer::cli
