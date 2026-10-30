// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "escape.hpp"

#include "cli/color.hpp"

#include <cstddef>
#include <string>
#include <string_view>

namespace cartomancer::render
{

namespace
{

[[nodiscard]] constexpr bool is_parameter(char one) noexcept
{
    return one >= '\x30' && one <= '\x3F';
}

[[nodiscard]] constexpr bool is_intermediate(char one) noexcept
{
    return one >= '\x20' && one <= '\x2F';
}

[[nodiscard]] constexpr bool is_final(char one) noexcept
{
    return one >= '\x40' && one <= '\x7E';
}

}  // namespace

std::size_t escape_length(std::string_view text) noexcept
{
    if (text.empty() || text.front() != cli::sgr_escape)
        return 0;

    // A lone ESC, or one introducing something that is not CSI. Consuming just
    // the ESC keeps us from reading past the end of the input.
    if (text.size() < 2 || text[1] != '[')
        return 1;

    std::size_t at = 2;
    while (at < text.size() && is_parameter(text[at])) ++at;
    while (at < text.size() && is_intermediate(text[at])) ++at;

    // Truncated: everything that is there belongs to the sequence.
    if (at >= text.size() || !is_final(text[at]))
        return text.size();

    return at + 1;
}

std::string strip_escapes(std::string_view text)
{
    std::string plain;
    plain.reserve(text.size());

    for (std::size_t at = 0; at < text.size();)
    {
        if (auto const skip = escape_length(text.substr(at)); skip > 0)
        {
            at += skip;
            continue;
        }

        plain += text[at];
        ++at;
    }

    return plain;
}

std::size_t visible_width(std::string_view text) noexcept
{
    std::size_t columns = 0;

    for (std::size_t at = 0; at < text.size();)
    {
        if (auto const skip = escape_length(text.substr(at)); skip > 0)
        {
            at += skip;
            continue;
        }

        columns += cli::display_width(text.substr(at, 1));
        ++at;
    }

    return columns;
}

}  // namespace cartomancer::render
