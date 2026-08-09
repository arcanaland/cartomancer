// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <arcana/validation.hpp>

#include <cstdint>
#include <optional>
#include <string_view>

namespace cartomancer
{

// --color WHEN. `--no-color` is an exact alias for `--color=never`.
//
// 256 and truecolor differ only for the card renderer, which does not exist
// yet; both mean "colour is on" for diagnostics.
enum class color_mode : std::uint8_t
{
    automatic,
    always,
    never,
    indexed_256,
    truecolor,
};

[[nodiscard]] std::optional<color_mode> parse_color_mode(std::string_view when) noexcept;

// Resolve --color against NO_COLOR and whether stdout is a terminal.
//
// An explicit --color beats the ambient NO_COLOR; NO_COLOR with any non-empty
// value beats the terminal check.
//
// @param mode        what --color / --no-color asked for
// @param explicitly_set whether either flag was actually given
// @param no_color    the NO_COLOR environment variable, if present
// @param tty         whether the output stream is a terminal
[[nodiscard]] bool resolve_color(
    color_mode mode, bool explicitly_set, std::optional<std::string_view> no_color, bool tty
) noexcept;

// The SGR sequence a severity is printed in, or "" when colour is off.
[[nodiscard]] std::string_view severity_color(arcana::severity level, bool colored) noexcept;

// The SGR reset, or "" when colour is off.
[[nodiscard]] std::string_view color_reset(bool colored) noexcept;

}  // namespace cartomancer
