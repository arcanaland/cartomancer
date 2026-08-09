// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "parse.hpp"

#include "surface.hpp"

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

// Handle one `--flag`.
//
// @return an error message when the flag or its value is not recognised.
[[nodiscard]] std::optional<std::string> apply_flag(
    token const& spelled, cursor& args, options& opts, parse_state& state
)
{
    flag const* const found = find_flag(spelled.name);
    if (found == nullptr)
        return std::format("unknown flag: {}", spelled.name);

    // Reject before consuming: a flag that takes no value must not swallow the
    // word after it.
    if (!found->takes_value())
        return found->apply("", opts, state);

    auto const value = args.value_for(spelled);
    if (!value.has_value())
        return std::format("{} needs a value", spelled.name);

    return found->apply(*value, opts, state);
}

// Handle one bare word: the subcommand if we have not seen one, else TARGET.
[[nodiscard]] std::optional<std::string> apply_positional(
    std::string_view word, options& opts, bool& saw_command
)
{
    if (!saw_command)
    {
        subcommand const* const which = find_subcommand(word);
        if (which == nullptr)
            return std::format("unknown subcommand: {}", word);

        opts.which = which->which;
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
            auto const spelled = split(argument);
            failure = apply_flag(spelled, walk, opts, state);
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
