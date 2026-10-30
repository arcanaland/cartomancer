// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// The fixed strings the CLI prints.

#pragma once

#include "color.hpp"
#include "streams.hpp"

#include <arcana/validation.hpp>

#include <string_view>

namespace cartomancer::cli
{

// The spelling of a severity in both the text and the JSON output.
[[nodiscard]] std::string_view severity_name(arcana::severity level) noexcept;

// The spelling of a rule's implementation state in both outputs.
[[nodiscard]] std::string_view rule_state_name(arcana::rule_state state) noexcept;

// The `x cartomancer: <message>` line every command reports a failure on.
void write_problem(streams const& sink, std::string_view message);

// The --help text styled for a color depth.
[[nodiscard]] std::string_view usage_text(color_depth depth) noexcept;

}  // namespace cartomancer::cli
