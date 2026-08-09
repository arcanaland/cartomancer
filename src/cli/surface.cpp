// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "surface.hpp"

#include <format>

namespace cartomancer::cli
{

namespace
{

[[nodiscard]] std::string bad_value(std::string_view flag, std::string_view value)
{
    return std::format("unrecognised value for {}: {}", flag, value);
}

[[nodiscard]] std::optional<arcana::severity> parse_severity(std::string_view name) noexcept
{
    if (name == "pedantic")
        return arcana::severity::pedantic;
    if (name == "info")
        return arcana::severity::info;
    if (name == "warning")
        return arcana::severity::warning;
    if (name == "error")
        return arcana::severity::error;
    return std::nullopt;
}

[[nodiscard]] std::optional<output_format> parse_format_value(std::string_view name) noexcept
{
    if (name == "text")
        return output_format::text;
    if (name == "json")
        return output_format::json;
    return std::nullopt;
}

}  // namespace

std::optional<std::string> apply_deck(std::string_view value, options& opts, [[maybe_unused]] parse_state& state)
{
    opts.deck = std::string(value);
    return std::nullopt;
}

std::optional<std::string> apply_explain(std::string_view value, options& opts, [[maybe_unused]] parse_state& state)
{
    opts.explain = std::string(value);
    return std::nullopt;
}

std::optional<std::string> apply_format(std::string_view value, options& opts, [[maybe_unused]] parse_state& state)
{
    auto const format = parse_format_value(value);
    if (!format.has_value())
        return bad_value("--format", value);

    opts.format = *format;
    return std::nullopt;
}

std::optional<std::string> apply_level(std::string_view value, options& opts, [[maybe_unused]] parse_state& state)
{
    auto const level = parse_severity(value);
    if (!level.has_value())
        return bad_value("--level", value);

    opts.level = *level;
    return std::nullopt;
}

std::optional<std::string> apply_color(std::string_view value, options& opts, parse_state& state)
{
    auto const mode = parse_color_mode(value);
    if (!mode.has_value())
        return bad_value("--color", value);

    opts.color = *mode;
    state.color_set = true;
    return std::nullopt;
}

std::optional<std::string> apply_no_color([[maybe_unused]] std::string_view value, options& opts, parse_state& state)
{
    opts.color = color_mode::never;
    state.color_set = true;
    return std::nullopt;
}

std::optional<std::string> apply_list_codes([[maybe_unused]] std::string_view value, options& opts, [[maybe_unused]] parse_state& state)
{
    opts.list_codes = true;
    return std::nullopt;
}

std::optional<std::string> apply_version([[maybe_unused]] std::string_view value, options& opts, [[maybe_unused]] parse_state& state)
{
    opts.version = true;
    return std::nullopt;
}

std::optional<std::string> apply_help([[maybe_unused]] std::string_view value, options& opts, [[maybe_unused]] parse_state& state)
{
    opts.help = true;
    return std::nullopt;
}

flag const* find_flag(std::string_view name) noexcept
{
    for (auto const& one : flags)
        if (one.name == name)
            return &one;

    return nullptr;
}

subcommand const* find_subcommand(std::string_view word) noexcept
{
    for (auto const& one : subcommands)
        if (one.name == word)
            return &one;

    return nullptr;
}

}  // namespace cartomancer::cli
