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

// The --help text, styled for a depth.
//
// Depth is all --help needs: it uses no glyphs and does not wrap to width. All
// four variants are compile-time constants, so this allocates nothing.
[[nodiscard]] std::string_view usage_text(color_depth depth) noexcept;

}  // namespace cartomancer::cli
