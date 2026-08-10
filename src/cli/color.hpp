// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// The terminal presentation layer.
//
// Every SGR sequence in the program is spelled in this header and nowhere else;
// `just lint-escapes` enforces that. Call sites name a role — `t.style.accent` —
// never a code.

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

// How much colour the terminal turned out to accept. `ansi16` is the floor and
// the default: it maps roles onto the terminal's own sixteen colours, so light
// and dark themes both work unconfigured.
enum class color_depth : std::uint8_t
{
    none,
    ansi16,
    indexed_256,
    truecolor,
};

// The byte every SGR sequence opens with, exported so that code which has to
// *recognise* an escape does not have to spell one.
inline constexpr char sgr_escape = '\033';

// The semantic roles. Every member is empty at depth `none`, so no call site
// ever branches on whether colour is on.
struct palette
{
    // Bold: headings, deck names, the one thing on a line that matters.
    std::string_view strong;

    // Dim: paths, counts, units, column headers.
    std::string_view muted;

    // An identifier the user can type back: a deck directory name, a rule code.
    std::string_view accent;

    std::string_view success;
    std::string_view error;
    std::string_view warning;
    std::string_view info;
    std::string_view pedantic;

    std::string_view reset;
};

// The palette for a depth.
//
// constexpr and header-visible because cli/text.cpp constant-evaluates it to
// build the four --help variants.
//
// A deeper palette only refines the roles `ansi16` already expresses; it never
// introduces one that `ansi16` cannot, so no output differs in structure by
// depth, only in shade.
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

// The marks the text surfaces print. Independent of colour: a NO_COLOR terminal
// in a UTF-8 locale still gets the Unicode set.
struct glyphs
{
    std::string_view ok;
    std::string_view bad;
    std::string_view warn;
    std::string_view arrow;
    std::string_view ellipsis;

    // The em dash in `ok nano-tarot - no problems found`. In the set rather than
    // written into the line because the ASCII fallback needs a hyphen.
    std::string_view dash;
};

inline constexpr glyphs unicode_marks{
    .ok = "✓",
    .bad = "✗",
    .warn = "⚠",
    .arrow = "→",
    .ellipsis = "…",
    .dash = "—",
};

inline constexpr glyphs ascii_marks{
    .ok = "[ok]",
    .bad = "[x]",
    .warn = "[!]",
    .arrow = "->",
    .ellipsis = "...",
    .dash = "-",
};

// Everything a text surface needs to know about the terminal, resolved once in
// app.cpp and carried by `cli::streams`.
struct theme
{
    // Not redundant with `style`: usage_text dispatches on it, and a future
    // image renderer picks a quantizer by it, which the palette cannot tell it.
    color_depth depth = color_depth::none;

    palette style = for_depth(color_depth::none);

    glyphs mark = ascii_marks;

    // Columns available for output. **0 means unbounded**, and is what a non-tty
    // gets: piped output must not depend on the pipe reader's window.
    std::size_t width = 0;
};

[[nodiscard]] std::optional<color_mode> parse_color_mode(std::string_view when) noexcept;

// Resolve --color against NO_COLOR, whether stdout is a terminal, and what the
// terminal advertises.
//
// An explicit --color beats the ambient NO_COLOR, which beats tty-ness. A pure
// function: the environment and isatty are read in app.cpp.
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

// The glyph set for a locale: the Unicode marks when the first of LC_ALL,
// LC_CTYPE and LANG that is set names a UTF-8 codeset, the ASCII ones otherwise.
[[nodiscard]] glyphs glyphs_for_locale(
    std::optional<std::string_view> lc_all, std::optional<std::string_view> lc_ctype,
    std::optional<std::string_view> lang
) noexcept;

// The terminal's own idea of its width, from TIOCGWINSZ on stdout.
//
// The impure half of width detection; app.cpp feeds its answer to resolve_width.
[[nodiscard]] std::optional<std::size_t> window_columns() noexcept;

// COLUMNS when it parses as a positive integer, else the window's own answer,
// else 80 for a terminal and 0 — unbounded — for anything else.
[[nodiscard]] std::size_t resolve_width(
    bool tty, std::optional<std::string_view> columns, std::optional<std::size_t> window
) noexcept;

// The style a severity is printed in, or "" when colour is off.
[[nodiscard]] std::string_view severity_style(theme const& t, arcana::severity level) noexcept;

// The columns text occupies, counting each UTF-8 sequence as one.
//
// Wide characters are counted as one column too; correcting that is the image
// renderer's problem, and deck names are ASCII-ish in practice.
[[nodiscard]] std::size_t display_width(std::string_view text) noexcept;

// text, shortened to at most `limit` columns and ending in `ellipsis` when it
// had to be. An empty string when limit is 0.
[[nodiscard]] std::string fit(std::string_view text, std::size_t limit, std::string_view ellipsis);

}  // namespace cartomancer::cli
