// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "cli.hpp"

#include <format>
#include <utility>

namespace cartomancer
{

namespace
{

// One argument, split on the first '=' so that `--level=error` and
// `--level error` are the same flag.
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

// Walks the argument list, handing out the value that belongs to a flag.
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

    // The value for `flag`: whatever followed '=', else the next argument.
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
    return flag == "--deck" || flag == "--explain" || flag == "--format" || flag == "--level"
           || flag == "--color";
}

// State the flag loop threads through, beyond the options themselves.
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

// Handle a flag that takes a value, given that value.
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

    // Reject before consuming: an unknown flag must not swallow the argument
    // that follows it.
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

    // Two deck selectors mean a wrong belief about one of them, so this is a
    // usage error rather than a silent precedence win. ADR-009.
    if (opts.deck.has_value() && opts.target.has_value())
        return {
            .opts = opts,
            .error = "pass either --deck NAME or a positional TARGET, not both",
        };

    opts.color_explicit = state.color_set;

    return {.opts = opts, .error = std::nullopt};
}

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
    return R"(cartomancer - view, validate and manage tarot decks

Usage:
  cartomancer <command> [flags] [TARGET]

Commands:
  validate [TARGET]   judge a deck against the arcana rule catalogue
  list                list the decks installed on this system

Validate flags:
  --format text|json                  output format (default text)
  --level pedantic|info|warning|error  report and exit on this floor (default info)
  --explain CODE                      print one catalogue entry and exit
  --list-codes                        print the whole catalogue and exit

List flags:
  --format text|json                  output format (default text)

Global flags:
  --deck NAME         act on a discovered deck by directory name
  --color WHEN        auto|always|never|256|truecolor (default auto)
  --no-color          alias for --color=never
  --version           print the cartomancer and libarcana versions
  --help              print this text

TARGET is a deck directory, or a discovered deck's directory name. Passing
both --deck and TARGET is a usage error.

Exit codes:
  0  no diagnostics at or above --level
  1  warnings at or above --level, and no errors
  2  at least one error diagnostic
  3  the deck could not be loaded at all
  4  usage error
)";
}

}  // namespace cartomancer
