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

// A whole command line, or the one message explaining why it is not one.
//
// Nothing partial escapes a failed parse: the parser stops at the first bad
// argument, so a half-filled `options` would depend on where in the line the
// mistake fell. Everything downstream of a usage error is a fixed diagnostic
// and an exit 4.
using parse_result = std::expected<options, std::string>;

// @param args argv without argv[0].
[[nodiscard]] parse_result parse(std::span<std::string_view const> args);

}  // namespace cartomancer::cli
