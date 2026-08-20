// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "text.hpp"

#include "surface.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <ranges>
#include <string>

namespace cartomancer::cli
{

namespace
{

// the left column of one --help line.
[[nodiscard]] constexpr std::size_t spelling_width(
    std::string_view name, std::string_view metavar
) noexcept
{
    return name.size() + (metavar.empty() ? 0 : 1 + metavar.size());
}

// The widest left column in `table`, which must not be empty.
[[nodiscard]] constexpr std::size_t widest_spelling(auto const& table)
{
    return std::ranges::max(
        table |
        std::views::transform([](auto const& one) { return spelling_width(one.name, one.metavar); })
    );
}

inline constexpr std::size_t flag_column = widest_spelling(flags) + 2;
inline constexpr std::size_t command_column = widest_spelling(subcommands) + 3;

constexpr void append_heading(std::string& out, std::string_view heading, palette const& style)
{
    out += style.strong;
    out += heading;
    out += style.reset;
    out += '\n';
}

// `  --format text|json      output format`, padded to `column`.
constexpr void append_entry(
    std::string& out, std::string_view name, std::string_view metavar, std::size_t column,
    std::string_view help, palette const& style
)
{
    out += "  ";
    out += style.accent;
    out += name;
    out += style.reset;

    if (!metavar.empty())
    {
        out += ' ';
        out += style.muted;
        out += metavar;
        out += style.reset;
    }

    auto const used = spelling_width(name, metavar);
    out.append(column > used ? column - used : 1, ' ');

    out += help;
    out += '\n';
}

// The flags in `which` in table order.
constexpr void append_flags(std::string& out, help_section which, palette const& style)
{
    for (auto const& one : flags)
        if (has_section(one.sections, which))
            append_entry(out, one.name, one.metavar, flag_column, one.help, style);
}

[[nodiscard]] constexpr std::string build_usage(palette const& style)
{
    std::string out = "cartomancer - swiss-army knife for tarot decks\n\n";

    append_heading(out, "Usage:", style);
    out += "  cartomancer <command> [flags] [TARGET]\n\n";

    append_heading(out, "Commands:", style);
    for (auto const& one : subcommands)
        append_entry(out, one.name, one.metavar, command_column, one.help, style);

    out += '\n';
    append_heading(out, "Validate flags:", style);
    append_flags(out, help_section::validate, style);

    out += '\n';
    append_heading(out, "List flags:", style);
    append_flags(out, help_section::list, style);

    out += '\n';
    append_heading(out, "Global flags:", style);
    append_flags(out, help_section::global, style);

    out += R"(
TARGET is a deck directory, or a discovered deck's directory name. Passing
both --deck and TARGET is a usage error.

)";

    append_heading(out, "Exit codes:", style);
    out += R"(  0  no diagnostics at or above --level
  1  warnings at or above --level, and no errors
  2  at least one error diagnostic
  3  the deck could not be loaded at all
  4  usage error
)";

    return out;
}

// NOLINTBEGIN(modernize-avoid-c-style-cast):
// clang-tidy gets confused here
template <color_depth Depth>
constexpr auto usage_storage = []
{
    constexpr palette style = for_depth(Depth);
    constexpr std::size_t size = build_usage(style).size();

    std::array<char, size> buffer{};
    std::string const text = build_usage(style);
    std::ranges::copy(text, buffer.begin());

    return buffer;
}();
// NOLINTEND(modernize-avoid-c-style-cast)

template <color_depth Depth>
[[nodiscard]] constexpr std::string_view usage_of() noexcept
{
    return {usage_storage<Depth>.data(), usage_storage<Depth>.size()};
}

template <color_depth Depth>
[[nodiscard]] constexpr bool aligns_like_plain()
{
    auto const styled = usage_of<Depth>();

    std::string stripped;
    for (std::size_t index = 0; index < styled.size(); ++index)
    {
        if (styled[index] == sgr_escape)
        {
            while (index < styled.size() && styled[index] != 'm') ++index;
            continue;
        }

        stripped += styled[index];
    }

    return std::ranges::equal(stripped, usage_of<color_depth::none>());
}

static_assert(!usage_of<color_depth::none>().empty());
static_assert(!usage_of<color_depth::ansi16>().empty());
static_assert(!usage_of<color_depth::indexed_256>().empty());
static_assert(!usage_of<color_depth::truecolor>().empty());

static_assert(aligns_like_plain<color_depth::ansi16>(), "--help padding counted the SGR bytes");
static_assert(
    aligns_like_plain<color_depth::indexed_256>(), "--help padding counted the SGR bytes"
);
static_assert(aligns_like_plain<color_depth::truecolor>(), "--help padding counted the SGR bytes");

}  // namespace

std::string_view severity_name(arcana::severity level) noexcept
{
    switch (level)
    {
        case arcana::severity::pedantic:
            return "pedantic";
        case arcana::severity::info:
            return "info";
        case arcana::severity::warning:
            return "warning";
        case arcana::severity::error:
            return "error";
    }
    return "unknown";
}

std::string_view rule_state_name(arcana::rule_state state) noexcept
{
    switch (state)
    {
        case arcana::rule_state::checked:
            return "checked";
        case arcana::rule_state::pending:
            return "pending";
        case arcana::rule_state::deferred:
            return "deferred";
    }
    return "unknown";
}

std::string_view usage_text(color_depth depth) noexcept
{
    switch (depth)
    {
        case color_depth::none:
            return usage_of<color_depth::none>();
        case color_depth::ansi16:
            return usage_of<color_depth::ansi16>();
        case color_depth::indexed_256:
            return usage_of<color_depth::indexed_256>();
        case color_depth::truecolor:
            return usage_of<color_depth::truecolor>();
    }

    return usage_of<color_depth::none>();
}

}  // namespace cartomancer::cli
