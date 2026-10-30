// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "color.hpp"

#include <sys/ioctl.h>
#include <unistd.h>
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <format>
#include <limits>

namespace cartomancer::cli
{

namespace
{

// What a terminal is assumed to be when it has a window but will not say how wide.
constexpr std::size_t fallback_width = 80;

// The classic terminal height, for when nothing will say.
constexpr std::size_t fallback_lines = 24;

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

namespace
{

// What TIOCGWINSZ says about stdout, if it says anything.
[[nodiscard]] std::optional<winsize> query_window() noexcept
{
    winsize window{};
    if (::ioctl(STDOUT_FILENO, TIOCGWINSZ, &window) != 0)
        return std::nullopt;

    return window;
}

}  // namespace

std::optional<std::size_t> window_columns() noexcept
{
    auto const window = query_window();
    if (!window.has_value() || window->ws_col == 0)
        return std::nullopt;

    return window->ws_col;
}

std::optional<std::size_t> window_rows() noexcept
{
    auto const window = query_window();
    if (!window.has_value() || window->ws_row == 0)
        return std::nullopt;

    return window->ws_row;
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

std::size_t resolve_lines(
    std::optional<std::string_view> lines, std::optional<std::size_t> window
) noexcept
{
    if (auto const asked = whole_number(lines.value_or(std::string_view{})); asked > 0)
        return asked;

    return window.value_or(fallback_lines);
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


namespace
{

// The six levels an xterm 6x6x6 cube axis takes.
constexpr std::array<int, 6> cube_levels{0, 95, 135, 175, 215, 255};

// The first index of the cube, and of the grey ramp above it.
constexpr int cube_base = 16;
constexpr int grey_base = 232;
constexpr int grey_steps = 24;
constexpr int grey_first = 8;
constexpr int grey_stride = 10;

// The sixteen ANSI colors as xterm renders them.
constexpr std::array<rgb, 16> ansi_palette{
    rgb{0, 0, 0},       rgb{205, 0, 0},   rgb{0, 205, 0},   rgb{205, 205, 0},
    rgb{0, 0, 238},     rgb{205, 0, 205}, rgb{0, 205, 205}, rgb{229, 229, 229},
    rgb{127, 127, 127}, rgb{255, 0, 0},   rgb{0, 255, 0},   rgb{255, 255, 0},
    rgb{92, 92, 255},   rgb{255, 0, 255}, rgb{0, 255, 255}, rgb{255, 255, 255},
};

// Where the sixteen split into the 30-37 and 90-97 halves.
constexpr std::uint8_t bright_first = 8;

constexpr int foreground_dim = 30;
constexpr int foreground_bright = 90;
constexpr int background_dim = 40;
constexpr int background_bright = 100;

[[nodiscard]] constexpr int channel(rgb color, std::size_t which) noexcept
{
    switch (which)
    {
        case 0:
            return color.red;
        case 1:
            return color.green;
        default:
            return color.blue;
    }
}

[[nodiscard]] constexpr int squared_distance(rgb left, rgb right) noexcept
{
    int total = 0;
    for (std::size_t axis = 0; axis < 3; ++axis)
    {
        int const gap = channel(left, axis) - channel(right, axis);
        total += gap * gap;
    }
    return total;
}

// The cube axis whose level is nearest to one channel.
[[nodiscard]] constexpr int nearest_level(int value) noexcept
{
    int best = 0;
    int closest = std::numeric_limits<int>::max();

    for (int step = 0; step < static_cast<int>(cube_levels.size()); ++step)
    {
        int const gap = std::abs(cube_levels[static_cast<std::size_t>(step)] - value);
        if (gap < closest)
        {
            closest = gap;
            best = step;
        }
    }

    return best;
}

[[nodiscard]] constexpr rgb level_color(int red, int green, int blue) noexcept
{
    return {
        static_cast<std::uint8_t>(cube_levels[static_cast<std::size_t>(red)]),
        static_cast<std::uint8_t>(cube_levels[static_cast<std::size_t>(green)]),
        static_cast<std::uint8_t>(cube_levels[static_cast<std::size_t>(blue)]),
    };
}

// The escape parameters that set one color, e.g. "38;2;1;2;3".
[[nodiscard]] std::string color_parameters(color_depth depth, rgb color, bool background)
{
    switch (depth)
    {
        case color_depth::truecolor:
            return std::format(
                "{};2;{};{};{}", background ? 48 : 38, color.red, color.green, color.blue
            );

        case color_depth::indexed_256:
            return std::format("{};5;{}", background ? 48 : 38, index_256(color));

        case color_depth::ansi16:
        {
            auto const slot = index_16(color);
            int const base = slot < bright_first
                                 ? (background ? background_dim : foreground_dim)
                                 : (background ? background_bright : foreground_bright);
            int const offset = slot < bright_first ? slot : slot - bright_first;
            return std::format("{}", base + offset);
        }

        case color_depth::none:
            break;
    }

    return {};
}

}  // namespace

std::uint8_t index_256(rgb color) noexcept
{
    auto const red = nearest_level(color.red);
    auto const green = nearest_level(color.green);
    auto const blue = nearest_level(color.blue);

    auto const from_cube = level_color(red, green, blue);
    auto const cube_index = static_cast<std::uint8_t>(cube_base + (36 * red) + (6 * green) + blue);
    auto const cube_gap = squared_distance(color, from_cube);

    // The grey nearest a color is the one nearest its mean channel: the other
    // two terms of the distance do not depend on which grey we pick.
    auto const mean = (color.red + color.green + color.blue) / 3;
    auto step = (mean - grey_first + (grey_stride / 2)) / grey_stride;
    step = std::clamp(step, 0, grey_steps - 1);

    auto const grey = static_cast<std::uint8_t>(grey_first + (grey_stride * step));
    auto const grey_gap = squared_distance(color, rgb{grey, grey, grey});

    return grey_gap < cube_gap ? static_cast<std::uint8_t>(grey_base + step) : cube_index;
}

std::uint8_t index_16(rgb color) noexcept
{
    std::uint8_t best = 0;
    int closest = std::numeric_limits<int>::max();

    for (std::size_t slot = 0; slot < ansi_palette.size(); ++slot)
    {
        int const gap = squared_distance(color, ansi_palette[slot]);
        if (gap < closest)
        {
            closest = gap;
            best = static_cast<std::uint8_t>(slot);
        }
    }

    return best;
}

void sgr_writer::paint(std::string& out, rgb foreground, rgb background)
{
    if (depth_ == color_depth::none)
        return;

    std::string parameters;

    if (foreground_ != foreground)
    {
        parameters = color_parameters(depth_, foreground, /*background=*/false);
        foreground_ = foreground;
    }

    if (background_ != background)
    {
        if (!parameters.empty())
            parameters += ';';

        parameters += color_parameters(depth_, background, /*background=*/true);
        background_ = background;
    }

    if (parameters.empty())
        return;

    out += sgr_escape;
    out += '[';
    out += parameters;
    out += 'm';
}

void sgr_writer::clear(std::string& out)
{
    if (!foreground_.has_value() && !background_.has_value())
        return;

    foreground_.reset();
    background_.reset();

    out += "\033[0m";
}

}  // namespace cartomancer::cli
