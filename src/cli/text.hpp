// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// The fixed strings the CLI prints.

#pragma once

#include "color.hpp"

#include <arcana/validation.hpp>

#include <string_view>

namespace cartomancer::cli
{

// The spelling of a severity in both the text and the JSON output.
[[nodiscard]] std::string_view severity_name(arcana::severity level) noexcept;

// The --help text styled for a color depth.
[[nodiscard]] std::string_view usage_text(color_depth depth) noexcept;

}  // namespace cartomancer::cli
