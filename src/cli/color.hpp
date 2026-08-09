// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <arcana/validation.hpp>

#include <cstdint>
#include <optional>
#include <string_view>

namespace cartomancer::cli
{
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
// An explicit --color beats the ambient NO_COLOR
//
// @param mode        what --color / --no-color asked for
// @param explicitly_set whether either flag was actually given
// @param no_color    the NO_COLOR environment variable, if present
// @param tty         whether the output stream is a terminal
[[nodiscard]] bool resolve_color(
    color_mode mode, bool explicitly_set, std::optional<std::string_view> no_color, bool tty
) noexcept;

// The SGR sequence a severity is printed in or "" when colour is off.
[[nodiscard]] std::string_view severity_color(arcana::severity level, bool use_color) noexcept;

// The SGR reset or "" when colour is off.
[[nodiscard]] std::string_view color_reset(bool use_color) noexcept;

}  // namespace cartomancer::cli
