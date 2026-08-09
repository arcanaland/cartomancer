// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// The command-line surface as data.

#pragma once

#include "options.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <string>
#include <string_view>

namespace cartomancer::cli
{

enum class help_section : std::uint8_t
{
    none = 0,
    validate = 1U << 0U,
    list = 1U << 1U,
    global = 1U << 2U,
};

[[nodiscard]] constexpr help_section operator|(help_section left, help_section right) noexcept
{
    return static_cast<help_section>(
        static_cast<std::uint8_t>(left) | static_cast<std::uint8_t>(right)
    );
}

[[nodiscard]] constexpr bool has_section(help_section set, help_section one) noexcept
{
    return (static_cast<std::uint8_t>(set) & static_cast<std::uint8_t>(one)) != 0U;
}

// Parser bookkeeping that is not itself part of the parsed result.
struct parse_state
{
    bool color_set = false;
};

using outcome = std::expected<void, std::string>;

// Applies one flag to opts.
//
// value is empty for a flag that takes none
//
// @return an error message when the value is not one this flag accepts.
using apply_fn = outcome (*)(std::string_view value, options& opts, parse_state& state);

struct flag
{
    // Spelled with leading dashes
    std::string_view name;

    // How the value is named in --help
    std::string_view metavar;

    std::string_view help;

    help_section sections;

    apply_fn apply;

    // A flag takes a value if and only if it has a metavar to name it.
    [[nodiscard]] constexpr bool takes_value() const noexcept
    {
        return !metavar.empty();
    }
};

outcome apply_deck(std::string_view value, options& opts, parse_state& state);
outcome apply_explain(std::string_view value, options& opts, parse_state& state);
outcome apply_format(std::string_view value, options& opts, parse_state& state);
outcome apply_level(std::string_view value, options& opts, parse_state& state);
outcome apply_color(std::string_view value, options& opts, parse_state& state);
outcome apply_no_color(std::string_view value, options& opts, parse_state& state);
outcome apply_list_codes(std::string_view value, options& opts, parse_state& state);
outcome apply_version(std::string_view value, options& opts, parse_state& state);
outcome apply_help(std::string_view value, options& opts, parse_state& state);

// Declaration order is --help order within each block.
inline constexpr std::array flags{
    flag{
        .name = "--format",
        .metavar = "text|json",
        .help = "output format (default text)",
        .sections = help_section::validate | help_section::list,
        .apply = apply_format,
    },
    flag{
        .name = "--level",
        .metavar = "pedantic|info|warning|error",
        .help = "report and exit on this floor (default info)",
        .sections = help_section::validate,
        .apply = apply_level,
    },
    flag{
        .name = "--explain",
        .metavar = "CODE",
        .help = "print one catalogue entry and exit",
        .sections = help_section::validate,
        .apply = apply_explain,
    },
    flag{
        .name = "--list-codes",
        .metavar = "",
        .help = "print the whole catalogue and exit",
        .sections = help_section::validate,
        .apply = apply_list_codes,
    },
    flag{
        .name = "--deck",
        .metavar = "NAME",
        .help = "act on a discovered deck by directory name",
        .sections = help_section::global,
        .apply = apply_deck,
    },
    flag{
        .name = "--color",
        .metavar = "WHEN",
        .help = "auto|always|never|256|truecolor (default auto)",
        .sections = help_section::global,
        .apply = apply_color,
    },
    flag{
        .name = "--no-color",
        .metavar = "",
        .help = "alias for --color=never",
        .sections = help_section::global,
        .apply = apply_no_color,
    },
    flag{
        .name = "--version",
        .metavar = "",
        .help = "print the cartomancer and libarcana versions",
        .sections = help_section::global,
        .apply = apply_version,
    },
    flag{
        .name = "--help",
        .metavar = "",
        .help = "print this text",
        .sections = help_section::global,
        .apply = apply_help,
    },
};

struct subcommand
{
    std::string_view name;

    // The positional the subcommand accepts
    std::string_view metavar;

    std::string_view help;

    command which;
};

inline constexpr std::array subcommands{
    subcommand{
        .name = "validate",
        .metavar = "[TARGET]",
        .help = "validate a deck against the tarot deck spec",
        .which = command::validate,
    },
    subcommand{
        .name = "list",
        .metavar = "",
        .help = "list the decks installed on this system",
        .which = command::list,
    },
};

consteval bool flags_are_well_formed()
{
    for (auto const& one : flags)
    {
        if (!one.name.starts_with("--") || one.name.size() <= 2)
            return false;
        if (one.help.empty() || one.apply == nullptr)
            return false;
        if (one.sections == help_section::none)
            return false;
    }

    for (std::size_t i = 0; i < flags.size(); ++i)
        for (std::size_t j = i + 1; j < flags.size(); ++j)
            if (flags[i].name == flags[j].name)
                return false;

    return true;
}

consteval bool subcommands_are_well_formed()
{
    for (auto const& one : subcommands)
        if (one.name.empty() || one.help.empty() || one.which == command::none)
            return false;

    for (std::size_t i = 0; i < subcommands.size(); ++i)
        for (std::size_t j = i + 1; j < subcommands.size(); ++j)
            if (subcommands[i].name == subcommands[j].name ||
                subcommands[i].which == subcommands[j].which)
                return false;

    return true;
}

static_assert(flags_are_well_formed(), "a flag is undocumented, unreachable or duplicated");

static_assert(
    subcommands_are_well_formed(), "a subcommand is undocumented, duplicated or dispatches nowhere"
);

static_assert(
    subcommands.size() == 2, "a command was added or removed without updating this assert"
);

[[nodiscard]] flag const* find_flag(std::string_view name) noexcept;

[[nodiscard]] subcommand const* find_subcommand(std::string_view word) noexcept;

}  // namespace cartomancer::cli
