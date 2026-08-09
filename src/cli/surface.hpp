// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// The command-line surface as data: every subcommand and flag the binary
// accepts, each carrying its own help text.
//
// The parser and --help both read these tables, so a flag cannot exist without
// being documented, and the documentation cannot describe a flag that does not
// exist. Adding a flag is one entry here and nothing else.

#pragma once

#include "options.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace cartomancer::cli
{

// Which --help block a flag is listed under. A set rather than one value
// because --format is documented under both subcommands.
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

// Applies one flag to `opts`.
//
// `value` is empty for a flag that takes none, which never reads it.
//
// @return an error message when the value is not one this flag accepts.
using apply_fn =
    std::optional<std::string> (*)(std::string_view value, options& opts, parse_state& state);

struct flag
{
    // Spelled with the leading dashes, as it appears on the command line.
    std::string_view name;

    // How the value is named in --help, or "" for a flag that takes none.
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

std::optional<std::string> apply_deck(std::string_view value, options& opts, parse_state& state);
std::optional<std::string> apply_explain(std::string_view value, options& opts, parse_state& state);
std::optional<std::string> apply_format(std::string_view value, options& opts, parse_state& state);
std::optional<std::string> apply_level(std::string_view value, options& opts, parse_state& state);
std::optional<std::string> apply_color(std::string_view value, options& opts, parse_state& state);
std::optional<std::string> apply_no_color(
    std::string_view value, options& opts, parse_state& state
);
std::optional<std::string> apply_list_codes(
    std::string_view value, options& opts, parse_state& state
);
std::optional<std::string> apply_version(std::string_view value, options& opts, parse_state& state);
std::optional<std::string> apply_help(std::string_view value, options& opts, parse_state& state);

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

    // The positional the subcommand accepts, as it appears in --help.
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

// Every flag is spelled as a long flag, is documented, is reachable from some
// --help block, and appears once. A table entry that fails any of these would
// otherwise produce a flag that works but is invisible, or one listed twice.
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

static_assert(flags_are_well_formed(), "a flag is undocumented, unreachable, or duplicated");

static_assert(
    subcommands_are_well_formed(), "a subcommand is undocumented, duplicated, or dispatches nowhere"
);

// A tripwire, not a derivation: nothing here can enumerate `command`, so an
// enumerator added without a spelling would otherwise be unreachable and
// undocumented in silence. Bumping this number is the prompt to add the entry.
// P2996's `enumerators_of(^^command)` would make this check real.
static_assert(subcommands.size() == 2, "a command was added or removed; is it in the table?");

// The flag whose spelling is `name`, or nullptr when it is not one of ours.
[[nodiscard]] flag const* find_flag(std::string_view name) noexcept;

// The subcommand `word` names, or nullptr when it is not one of ours.
[[nodiscard]] subcommand const* find_subcommand(std::string_view word) noexcept;

}  // namespace cartomancer::cli
