// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// The terminal presentation layer.

#pragma once

#include <arcana/validation.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace cartomancer::cli
{

// What --color asked for.
enum class color_mode : std::uint8_t
{
    automatic,
    always,
    never,
    indexed_256,
    truecolor,
};

// How much color the terminal turned out to accept.
enum class color_depth : std::uint8_t
{
    none,
    ansi16,
    indexed_256,
    truecolor,
};

inline constexpr char sgr_escape = '\033';

struct palette
{
    // Bold (headings, deck names, etc)
    std::string_view strong;

    // Dim (paths, counts, units, column headers)
    std::string_view muted;

    std::string_view accent;

    std::string_view success;
    std::string_view error;
    std::string_view warning;
    std::string_view info;
    std::string_view pedantic;

    std::string_view reset;
};

[[nodiscard]] constexpr palette for_depth(color_depth depth) noexcept
{
    switch (depth)
    {
        case color_depth::none:
            return {};

        case color_depth::ansi16:
            return {
                .strong = "\033[1m",
                .muted = "\033[2m",
                .accent = "\033[36m",
                .success = "\033[32m",
                .error = "\033[1;31m",
                .warning = "\033[1;33m",
                .info = "\033[1;36m",
                .pedantic = "\033[2m",
                .reset = "\033[0m",
            };

        case color_depth::indexed_256:
            return {
                .strong = "\033[1m",
                .muted = "\033[38;5;245m",
                .accent = "\033[38;5;44m",
                .success = "\033[38;5;77m",
                .error = "\033[1;38;5;203m",
                .warning = "\033[38;5;214m",
                .info = "\033[38;5;81m",
                .pedantic = "\033[38;5;245m",
                .reset = "\033[0m",
            };

        case color_depth::truecolor:
            return {
                .strong = "\033[1m",
                .muted = "\033[38;2;136;136;136m",
                .accent = "\033[38;2;38;198;218m",
                .success = "\033[38;2;76;175;80m",
                .error = "\033[1;38;2;239;83;80m",
                .warning = "\033[38;2;255;167;38m",
                .info = "\033[38;2;66;165;245m",
                .pedantic = "\033[38;2;136;136;136m",
                .reset = "\033[0m",
            };
    }

    return {};
}

// The marks the text surfaces print.
struct glyphs
{
    std::string_view ok;
    std::string_view bad;
    std::string_view warn;
    std::string_view arrow;
    std::string_view ellipsis;
    std::string_view dash;
};

inline constexpr glyphs unicode_marks{
    .ok = "✓",
    .bad = "✗",
    .warn = "⚠",
    .arrow = "→",
    .ellipsis = "…",
    .dash = "·",
};

inline constexpr glyphs ascii_marks{
    .ok = "[ok]",
    .bad = "[x]",
    .warn = "[!]",
    .arrow = "->",
    .ellipsis = "...",
    .dash = "-",
};

struct theme
{
    color_depth depth = color_depth::none;

    palette style = for_depth(color_depth::none);

    glyphs mark = ascii_marks;

    std::size_t width = 0;
};

[[nodiscard]] std::optional<color_mode> parse_color_mode(std::string_view when) noexcept;

// Resolve --color against NO_COLOR, whether stdout is a terminal, and what the
// terminal advertises.
//
// An explicit --color beats the ambient NO_COLOR, which beats tty-ness.
//
// @param mode           what --color / --no-color asked for
// @param explicitly_set whether either flag was actually given
// @param no_color       the NO_COLOR environment variable, if present
// @param tty            whether the output stream is a terminal
// @param colorterm      the COLORTERM environment variable, if present
// @param term           the TERM environment variable, if present
[[nodiscard]] color_depth resolve_color_depth(
    color_mode mode, bool explicitly_set, std::optional<std::string_view> no_color, bool tty,
    std::optional<std::string_view> colorterm, std::optional<std::string_view> term
) noexcept;

// The glyph set for a locale
[[nodiscard]] glyphs glyphs_for_locale(
    std::optional<std::string_view> lc_all, std::optional<std::string_view> lc_ctype,
    std::optional<std::string_view> lang
) noexcept;

// The terminal's own idea of its width
[[nodiscard]] std::optional<std::size_t> window_columns() noexcept;

[[nodiscard]] std::size_t resolve_width(
    bool tty, std::optional<std::string_view> columns, std::optional<std::size_t> window
) noexcept;

// The style a severity is printed in
[[nodiscard]] std::string_view severity_style(theme const& t, arcana::severity level) noexcept;

// The style a rule's implementation state is printed in. A rule nothing checks
// is dimmed, because a reader scanning the catalogue should see at a glance
// which rows can actually say anything about a deck.
[[nodiscard]] std::string_view rule_state_style(theme const& t, arcana::rule_state state) noexcept;

[[nodiscard]] std::size_t display_width(std::string_view text) noexcept;

// text, shortened to at most limit columns and ending in ellipsis when it
// can't fit
[[nodiscard]] std::string fit(std::string_view text, std::size_t limit, std::string_view ellipsis);

}  // namespace cartomancer::cli
