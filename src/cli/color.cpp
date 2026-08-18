// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "color.hpp"

#include <sys/ioctl.h>
#include <unistd.h>
#include <algorithm>
#include <cctype>

namespace cartomancer::cli
{

namespace
{

// What a terminal is assumed to be when it has a window but will not say how wide.
constexpr std::size_t fallback_width = 80;

constexpr unsigned char continuation_mask = 0xC0U;
constexpr unsigned char continuation_bits = 0x80U;

constexpr std::size_t decimal_base = 10;

[[nodiscard]] constexpr bool starts_codepoint(char one) noexcept
{
    return (static_cast<unsigned char>(one) & continuation_mask) != continuation_bits;
}

// text as a decimal count, or 0 for anything that is not one.
[[nodiscard]] std::size_t whole_number(std::string_view text) noexcept
{
    if (text.empty())
        return 0;

    std::size_t value = 0;
    for (char const one : text)
    {
        if (one < '0' || one > '9')
            return 0;

        value = (value * decimal_base) + static_cast<std::size_t>(one - '0');
    }

    return value;
}

// What the terminal advertises
[[nodiscard]] color_depth advertised_depth(
    std::optional<std::string_view> colorterm, std::optional<std::string_view> term
) noexcept
{
    if (colorterm.has_value() && (*colorterm == "truecolor" || *colorterm == "24bit"))
        return color_depth::truecolor;

    if (term.has_value() && term->contains("256color"))
        return color_depth::indexed_256;

    return color_depth::ansi16;
}

// The first of the locale variables that is set to something non-empty.
[[nodiscard]] std::string_view effective_locale(
    std::optional<std::string_view> lc_all, std::optional<std::string_view> lc_ctype,
    std::optional<std::string_view> lang
) noexcept
{
    for (auto const& candidate : {lc_all, lc_ctype, lang})
        if (candidate.has_value() && !candidate->empty())
            return *candidate;

    return {};
}

[[nodiscard]] bool names_utf8(std::string_view locale) noexcept
{
    auto const dot = locale.rfind('.');
    if (dot == std::string_view::npos)
        return false;

    auto codeset = locale.substr(dot + 1);

    // A modifier such as `@euro` is not part of the codeset.
    if (auto const modifier = codeset.find('@'); modifier != std::string_view::npos)
        codeset = codeset.substr(0, modifier);

    std::string folded;
    for (char const one : codeset)
        if (one != '-' && one != '_')
            folded += static_cast<char>(std::tolower(static_cast<unsigned char>(one)));

    return folded == "utf8";
}

}  // namespace

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

color_depth resolve_color_depth(
    color_mode mode, bool explicitly_set, std::optional<std::string_view> no_color, bool tty,
    std::optional<std::string_view> colorterm, std::optional<std::string_view> term
) noexcept
{
    // An explicit --color beats the ambient NO_COLOR
    if (explicitly_set)
    {
        switch (mode)
        {
            case color_mode::never:
                return color_depth::none;
            case color_mode::indexed_256:
                return color_depth::indexed_256;
            case color_mode::truecolor:
                return color_depth::truecolor;
            case color_mode::always:
                return advertised_depth(colorterm, term);
            case color_mode::automatic:
                return tty ? advertised_depth(colorterm, term) : color_depth::none;
        }
    }

    // https://no-color.org rule
    if (no_color.has_value() && !no_color->empty())
        return color_depth::none;

    return tty ? advertised_depth(colorterm, term) : color_depth::none;
}

glyphs glyphs_for_locale(
    std::optional<std::string_view> lc_all, std::optional<std::string_view> lc_ctype,
    std::optional<std::string_view> lang
) noexcept
{
    return names_utf8(effective_locale(lc_all, lc_ctype, lang)) ? unicode_marks : ascii_marks;
}

std::optional<std::size_t> window_columns() noexcept
{
    winsize window{};
    if (::ioctl(STDOUT_FILENO, TIOCGWINSZ, &window) != 0 || window.ws_col == 0)
        return std::nullopt;

    return window.ws_col;
}

std::size_t resolve_width(
    bool tty, std::optional<std::string_view> columns, std::optional<std::size_t> window
) noexcept
{
    if (auto const asked = whole_number(columns.value_or(std::string_view{})); asked > 0)
        return asked;

    if (!tty)
        return 0;

    return window.value_or(fallback_width);
}

std::string_view severity_style(theme const& t, arcana::severity level) noexcept
{
    switch (level)
    {
        case arcana::severity::error:
            return t.style.error;
        case arcana::severity::warning:
            return t.style.warning;
        case arcana::severity::info:
            return t.style.info;
        case arcana::severity::pedantic:
            return t.style.pedantic;
    }
    return {};
}

std::string_view rule_state_style(theme const& t, arcana::rule_state state) noexcept
{
    switch (state)
    {
        case arcana::rule_state::checked:
            return t.style.success;
        case arcana::rule_state::pending:
        case arcana::rule_state::deferred:
            return t.style.muted;
    }
    return {};
}

std::size_t display_width(std::string_view text) noexcept
{
    // Every byte but a UTF-8 continuation starts a new column.
    return static_cast<std::size_t>(std::ranges::count_if(text, starts_codepoint));
}

namespace
{

// The longest prefix of text that occupies at most columns cut on a
// codepoint boundary.
[[nodiscard]] std::string_view cut_to(std::string_view text, std::size_t columns) noexcept
{
    std::size_t used = 0;
    for (std::size_t index = 0; index < text.size(); ++index)
    {
        if (starts_codepoint(text[index]))
        {
            if (used == columns)
                return text.substr(0, index);
            ++used;
        }
    }
    return text;
}

}  // namespace

std::string fit(std::string_view text, std::size_t limit, std::string_view ellipsis)
{
    if (limit == 0)
        return {};

    if (display_width(text) <= limit)
        return std::string{text};

    // When even the ellipsis will not fit, a bare cut is all that is left.
    auto const marker = display_width(ellipsis);
    if (marker >= limit)
        return std::string{cut_to(text, limit)};

    return std::string{cut_to(text, limit - marker)} + std::string{ellipsis};
}

}  // namespace cartomancer::cli
