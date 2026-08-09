// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "color.hpp"

namespace cartomancer::cli
{

std::optional<color_mode> parse_color_mode(std::string_view when) noexcept
{
    if (when == "auto")
        return color_mode::automatic;
    if (when == "always")
        return color_mode::always;
    if (when == "never")
        return color_mode::never;
    if (when == "256")
        return color_mode::indexed_256;
    if (when == "truecolor")
        return color_mode::truecolor;

    return std::nullopt;
}

bool resolve_color(
    color_mode mode, bool explicitly_set, std::optional<std::string_view> no_color, bool tty
) noexcept
{
    // An explicit --color beats the ambient NO_COLOR
    if (explicitly_set)
        return mode == color_mode::automatic ? tty : mode != color_mode::never;

    // https://no-color.org rule
    if (no_color.has_value() && !no_color->empty())
        return false;

    return tty;
}

std::string_view severity_color(arcana::severity level, bool use_color) noexcept
{
    if (!use_color)
        return "";

    switch (level)
    {
        case arcana::severity::error:
            return "\033[1;31m";
        case arcana::severity::warning:
            return "\033[1;33m";
        case arcana::severity::info:
            return "\033[1;36m";
        case arcana::severity::pedantic:
            return "\033[2m";
    }
    return "";
}

std::string_view color_reset(bool use_color) noexcept
{
    return use_color ? "\033[0m" : "";
}

}  // namespace cartomancer::cli
