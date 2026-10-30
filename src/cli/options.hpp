// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include "color.hpp"

#include <arcana/validation.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace cartomancer::cli
{

enum class exit_code : std::uint8_t
{
    // No diagnostics at or above level.
    ok = 0,

    // Warnings at or above level and no errors.
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
    show,
    draw,
};

// A parsed command line
struct options
{
    command which = command::none;

    // --deck NAME
    std::optional<std::string> deck;

    // The positional TARGET
    std::optional<std::string> target;

    // show's positional CARD-ID, unparsed
    std::optional<std::string> card;

    // draw's positional N
    std::size_t count = 1;

    output_format format = output_format::text;

    // The --level floor
    arcana::severity level = arcana::severity::info;

    // --explain CODE
    std::optional<std::string> explain;

    bool list_codes = false;

    // --lines N, show's target height in rows
    std::optional<std::size_t> lines;

    // --no-art
    bool no_art = false;

    // --seed S
    std::optional<std::uint64_t> seed;

    color_mode color = color_mode::automatic;

    // Whether --color or --no-color was actually given.
    bool color_explicit = false;

    bool help = false;
    bool version = false;
};

}  // namespace cartomancer::cli
