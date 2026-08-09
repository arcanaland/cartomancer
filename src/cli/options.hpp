// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// The shape of a parsed command line. Kept apart from the parser so a command
// can take `options` without pulling in the argument walker.

#pragma once

#include "color.hpp"

#include <arcana/validation.hpp>

#include <cstdint>
#include <optional>
#include <string>

namespace cartomancer::cli
{

enum class exit_code : std::uint8_t
{
    // No diagnostics at or above --level.
    ok = 0,

    // Warnings at or above --level and no errors.
    warnings = 1,

    // At least one error diagnostic.
    errors = 2,

    // The deck could not be loaded at all.
    unloadable = 3,

    // Usage error
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
    none,
    validate,
    list,
};

// A parsed command line
struct options
{
    command which = command::none;

    // --deck NAME
    std::optional<std::string> deck;

    // The positional TARGET: a directory path, or a discovered deck's
    // directory name.
    std::optional<std::string> target;

    output_format format = output_format::text;

    // The --level floor
    // everything; we filter its result.
    arcana::severity level = arcana::severity::info;

    // --explain CODE
    std::optional<std::string> explain;

    bool list_codes = false;

    color_mode color = color_mode::automatic;

    // Whether --color or --no-color was actually given.
    bool color_explicit = false;

    bool help = false;
    bool version = false;
};

}  // namespace cartomancer::cli
