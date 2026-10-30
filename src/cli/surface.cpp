// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "surface.hpp"

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace cartomancer::cli
{

namespace
{

[[nodiscard]] std::unexpected<std::string> bad_value(std::string_view flag, std::string_view value)
{
    return std::unexpected(std::format("unrecognised value for {}: {}", flag, value));
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

// A bare decimal count, and nothing else: no sign, no separators, no
// whitespace. std::from_chars would accept a leading digit run of an
// otherwise nonsense argument.
template <typename Number>
[[nodiscard]] std::optional<Number> whole_number(std::string_view text) noexcept
{
    if (text.empty())
        return std::nullopt;

    Number value = 0;
    auto const [stop, failed] = std::from_chars(text.data(), text.data() + text.size(), value);

    if (failed != std::errc{} || stop != text.data() + text.size())
        return std::nullopt;

    return value;
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

outcome apply_deck(std::string_view value, options& opts, [[maybe_unused]] parse_state& state)
{
    opts.deck = std::string(value);
    return {};
}

outcome apply_explain(std::string_view value, options& opts, [[maybe_unused]] parse_state& state)
{
    opts.explain = std::string(value);
    return {};
}

outcome apply_format(std::string_view value, options& opts, [[maybe_unused]] parse_state& state)
{
    auto const format = parse_format_value(value);
    if (!format.has_value())
        return bad_value("--format", value);

    opts.format = *format;
    return {};
}

outcome apply_level(std::string_view value, options& opts, [[maybe_unused]] parse_state& state)
{
    auto const level = parse_severity(value);
    if (!level.has_value())
        return bad_value("--level", value);

    opts.level = *level;
    return {};
}

outcome apply_color(std::string_view value, options& opts, parse_state& state)
{
    auto const mode = parse_color_mode(value);
    if (!mode.has_value())
        return bad_value("--color", value);

    opts.color = *mode;
    state.color_set = true;
    return {};
}

outcome apply_no_color([[maybe_unused]] std::string_view value, options& opts, parse_state& state)
{
    opts.color = color_mode::never;
    state.color_set = true;
    return {};
}

outcome apply_list_codes(
    [[maybe_unused]] std::string_view value, options& opts, [[maybe_unused]] parse_state& state
)
{
    opts.list_codes = true;
    return {};
}

outcome apply_lines(std::string_view value, options& opts, [[maybe_unused]] parse_state& state)
{
    auto const rows = whole_number<std::size_t>(value);
    if (!rows.has_value() || *rows == 0)
        return bad_value("--lines", value);

    opts.lines = *rows;
    return {};
}

outcome apply_no_art(
    [[maybe_unused]] std::string_view value, options& opts, [[maybe_unused]] parse_state& state
)
{
    opts.no_art = true;
    return {};
}

outcome apply_seed(std::string_view value, options& opts, [[maybe_unused]] parse_state& state)
{
    auto const seed = whole_number<std::uint64_t>(value);
    if (!seed.has_value())
        return bad_value("--seed", value);

    opts.seed = *seed;
    return {};
}

outcome apply_version(
    [[maybe_unused]] std::string_view value, options& opts, [[maybe_unused]] parse_state& state
)
{
    opts.version = true;
    return {};
}

outcome apply_help(
    [[maybe_unused]] std::string_view value, options& opts, [[maybe_unused]] parse_state& state
)
{
    opts.help = true;
    return {};
}

std::optional<std::size_t> parse_count(std::string_view text) noexcept
{
    return whole_number<std::size_t>(text);
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

std::string_view name_of(command which) noexcept
{
    for (auto const& one : subcommands)
        if (one.which == which)
            return one.name;

    return {};
}

}  // namespace cartomancer::cli
