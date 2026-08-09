// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "text.hpp"

#include "surface.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>

namespace cartomancer::cli
{

namespace
{

// "--format text|json" -- the left column of one --help line.
[[nodiscard]] constexpr std::size_t spelling_width(
    std::string_view name, std::string_view metavar
) noexcept
{
    return name.size() + (metavar.empty() ? 0 : 1 + metavar.size());
}

// Every description starts in the same column, in every block, computed from
// the widest entry rather than counted out by hand.
[[nodiscard]] constexpr std::size_t flag_column()
{
    std::size_t widest = 0;
    for (auto const& one : flags) widest = std::max(widest, spelling_width(one.name, one.metavar));

    return widest + 2;
}

[[nodiscard]] constexpr std::size_t command_column()
{
    std::size_t widest = 0;
    for (auto const& one : subcommands)
        widest = std::max(widest, spelling_width(one.name, one.metavar));

    return widest + 3;
}

// `  --format text|json      output format`, padded to `column`.
constexpr void append_entry(
    std::string& out, std::string_view name, std::string_view metavar, std::size_t column,
    std::string_view help
)
{
    out += "  ";
    out += name;
    if (!metavar.empty())
    {
        out += ' ';
        out += metavar;
    }

    auto const used = spelling_width(name, metavar);
    out.append(column > used ? column - used : 1, ' ');

    out += help;
    out += '\n';
}

// The flags in `which`, one line each, in table order.
constexpr void append_flags(std::string& out, help_section which)
{
    for (auto const& one : flags)
        if (has_section(one.sections, which))
            append_entry(out, one.name, one.metavar, flag_column(), one.help);
}

[[nodiscard]] constexpr std::string build_usage()
{
    std::string out = R"(cartomancer - swiss-army knife for tarot decks

Usage:
  cartomancer <command> [flags] [TARGET]

Commands:
)";

    for (auto const& one : subcommands)
        append_entry(out, one.name, one.metavar, command_column(), one.help);

    out += "\nValidate flags:\n";
    append_flags(out, help_section::validate);

    out += "\nList flags:\n";
    append_flags(out, help_section::list);

    out += "\nGlobal flags:\n";
    append_flags(out, help_section::global);

    out += R"(
TARGET is a deck directory, or a discovered deck's directory name. Passing
both --deck and TARGET is a usage error.

Exit codes:
  0  no diagnostics at or above --level
  1  warnings at or above --level, and no errors
  2  at least one error diagnostic
  3  the deck could not be loaded at all
  4  usage error
)";

    return out;
}

// Laid out at compile time into static storage, so --help costs no allocation
// and usage_text() can honestly promise not to throw.
constexpr auto usage_storage = []
{
    constexpr std::size_t size = build_usage().size();

    std::array<char, size> buffer{};
    std::string const text = build_usage();
    std::ranges::copy(text, buffer.begin());

    return buffer;
}();

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

std::string_view usage_text() noexcept
{
    return {usage_storage.data(), usage_storage.size()};
}

}  // namespace cartomancer::cli
