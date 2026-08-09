// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "parse.hpp"

#include <format>
#include <utility>

namespace cartomancer::cli
{

namespace
{

// One argument split on the first =
struct token
{
    std::string_view name;
    std::optional<std::string_view> inline_value;
};

[[nodiscard]] token split(std::string_view argument)
{
    auto const equals = argument.find('=');
    if (equals == std::string_view::npos)
        return {.name = argument, .inline_value = std::nullopt};

    return {
        .name = argument.substr(0, equals),
        .inline_value = argument.substr(equals + 1),
    };
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

[[nodiscard]] std::optional<output_format> parse_format(std::string_view name) noexcept
{
    if (name == "text")
        return output_format::text;
    if (name == "json")
        return output_format::json;
    return std::nullopt;
}

[[nodiscard]] std::optional<command> parse_command(std::string_view name) noexcept
{
    if (name == "validate")
        return command::validate;
    if (name == "list")
        return command::list;
    return std::nullopt;
}

// Walks the argument list
class cursor
{
  public:
    explicit cursor(std::span<std::string_view const> args) : args_(args) {}

    [[nodiscard]] bool done() const
    {
        return index_ >= args_.size();
    }

    [[nodiscard]] std::string_view take()
    {
        return args_[index_++];
    }

    // The value for flag
    //
    // @return std::nullopt when the flag is at the end with nothing after it.
    [[nodiscard]] std::optional<std::string_view> value_for(token const& flag)
    {
        if (flag.inline_value.has_value())
            return flag.inline_value;
        if (done())
            return std::nullopt;
        return take();
    }

  private:
    std::span<std::string_view const> args_;
    std::size_t index_ = 0;
};

[[nodiscard]] std::string missing_value(std::string_view flag)
{
    return std::format("{} needs a value", flag);
}

[[nodiscard]] std::string bad_value(std::string_view flag, std::string_view value)
{
    return std::format("unrecognised value for {}: {}", flag, value);
}

[[nodiscard]] bool takes_value(std::string_view flag) noexcept
{
    return flag == "--deck" || flag == "--explain" || flag == "--format" || flag == "--level" ||
           flag == "--color";
}

struct parse_state
{
    bool color_set = false;
};

// Handle a flag that takes no value.
//
// @return true when `flag` was one of them.
[[nodiscard]] bool apply_toggle(token const& flag, options& opts, parse_state& state)
{
    if (flag.name == "--help")
    {
        opts.help = true;
        return true;
    }

    if (flag.name == "--version")
    {
        opts.version = true;
        return true;
    }

    if (flag.name == "--list-codes")
    {
        opts.list_codes = true;
        return true;
    }

    if (flag.name == "--no-color")
    {
        opts.color = color_mode::never;
        state.color_set = true;
        return true;
    }

    return false;
}

// Handle a flag that takes a value
//
// @return an error message when the value is not one this flag accepts.
[[nodiscard]] std::optional<std::string> apply_valued(
    token const& flag, std::string_view value, options& opts, parse_state& state
)
{
    if (flag.name == "--deck")
    {
        opts.deck = std::string(value);
        return std::nullopt;
    }

    if (flag.name == "--explain")
    {
        opts.explain = std::string(value);
        return std::nullopt;
    }

    if (flag.name == "--format")
    {
        auto const format = parse_format(value);
        if (!format.has_value())
            return bad_value(flag.name, value);
        opts.format = *format;
        return std::nullopt;
    }

    if (flag.name == "--level")
    {
        auto const level = parse_severity(value);
        if (!level.has_value())
            return bad_value(flag.name, value);
        opts.level = *level;
        return std::nullopt;
    }

    if (flag.name == "--color")
    {
        auto const mode = parse_color_mode(value);
        if (!mode.has_value())
            return bad_value(flag.name, value);
        opts.color = *mode;
        state.color_set = true;
        return std::nullopt;
    }

    std::unreachable();
}

// Handle one `--flag`.
//
// @return an error message when the flag or its value is not recognised.
[[nodiscard]] std::optional<std::string> apply_flag(
    token const& flag, cursor& args, options& opts, parse_state& state
)
{
    if (apply_toggle(flag, opts, state))
        return std::nullopt;

    // Reject before consuming
    if (!takes_value(flag.name))
        return std::format("unknown flag: {}", flag.name);

    auto const value = args.value_for(flag);
    if (!value.has_value())
        return missing_value(flag.name);

    return apply_valued(flag, *value, opts, state);
}

// Handle one bare word: the subcommand if we have not seen one, else TARGET.
[[nodiscard]] std::optional<std::string> apply_positional(
    std::string_view word, options& opts, bool& saw_command
)
{
    if (!saw_command)
    {
        auto const which = parse_command(word);
        if (!which.has_value())
            return std::format("unknown subcommand: {}", word);
        opts.which = *which;
        saw_command = true;
        return std::nullopt;
    }

    if (opts.target.has_value())
        return std::format("unexpected argument: {}", word);

    opts.target = std::string(word);
    return std::nullopt;
}

}  // namespace

parse_result parse(std::span<std::string_view const> args)
{
    options opts;
    parse_state state;
    cursor walk(args);
    bool saw_command = false;

    while (!walk.done())
    {
        auto const argument = walk.take();

        std::optional<std::string> failure;
        if (argument.starts_with('-') && argument != "-")
        {
            auto const flag = split(argument);
            failure = apply_flag(flag, walk, opts, state);
        }
        else
        {
            failure = apply_positional(argument, opts, saw_command);
        }

        if (failure.has_value())
            return {.opts = opts, .error = std::move(failure)};
    }

    if (opts.deck.has_value() && opts.target.has_value())
        return {
            .opts = opts,
            .error = "pass either --deck NAME or a positional TARGET, not both",
        };

    opts.color_explicit = state.color_set;

    return {.opts = opts, .error = std::nullopt};
}

}  // namespace cartomancer::cli
