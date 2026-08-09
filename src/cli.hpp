// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include "color.hpp"

#include <arcana/validation.hpp>

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace cartomancer
{

// ADR-009's exit-code contract. Every value here is API: org CI gates on it.
enum class exit_code : std::uint8_t
{
    // No diagnostics at or above --level.
    ok = 0,

    // Warnings at or above --level, and no errors.
    warnings = 1,

    // At least one error diagnostic.
    errors = 2,

    // The deck could not be loaded at all.
    unloadable = 3,

    // Usage error: unknown flag or subcommand, or two deck selectors.
    usage = 4,
};

[[nodiscard]] constexpr int to_int(exit_code code) noexcept
{
    return static_cast<int>(code);
}

enum class output_format : std::uint8_t
{
    text,
    json,
};

enum class command : std::uint8_t
{
    // No subcommand given: print help.
    none,
    validate,
    list,
};

// A parsed command line, or the usage error that stopped it being one.
struct options
{
    command which = command::none;

    // --deck NAME
    std::optional<std::string> deck;

    // The positional TARGET: a directory path, or a discovered deck's
    // directory name.
    std::optional<std::string> target;

    output_format format = output_format::text;

    // The --level floor, applied CLI-side. arcana::validate always runs
    // everything; we filter its result.
    arcana::severity level = arcana::severity::info;

    // --explain CODE
    std::optional<std::string> explain;

    bool list_codes = false;

    color_mode color = color_mode::automatic;

    // Whether --color or --no-color was actually given. An explicit flag beats
    // the ambient NO_COLOR.
    bool color_explicit = false;

    bool help = false;
    bool version = false;
};

struct parse_result
{
    options opts;

    // Set when the command line is not valid. `run` prints it and exits 4.
    std::optional<std::string> error;
};

[[nodiscard]] parse_result parse(std::span<std::string_view const> args);

[[nodiscard]] std::string_view severity_name(arcana::severity level) noexcept;

// The --help text, also printed for a bare `cartomancer`.
[[nodiscard]] std::string_view usage_text() noexcept;

}  // namespace cartomancer
