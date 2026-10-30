// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "parse.hpp"

#include "surface.hpp"

#include <cstddef>
#include <expected>
#include <format>
#include <optional>
#include <string>
#include <string_view>
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
[[nodiscard]] outcome apply_flag(
    token const& spelled, cursor& args, options& opts, parse_state& state
)
{
    flag const* const found = find_flag(spelled.name);
    if (found == nullptr)
        return std::unexpected(std::format("unknown flag: {}", spelled.name));

    state.seen.push_back(found->name);

    // Reject before consuming
    if (!found->takes_value())
        return found->apply("", opts, state);

    auto const value = args.value_for(spelled);
    if (!value.has_value())
        return std::unexpected(std::format("{} needs a value", spelled.name));

    return found->apply(*value, opts, state);
}

// How many bare words the subcommand has taken so far.
struct positionals
{
    bool saw_command = false;
    std::size_t taken = 0;
};

// One bare word after the subcommand. Which slot it fills is the subcommand's
// business: `show` takes CARD-ID then TARGET, `draw` takes N, and the rest
// take TARGET.
[[nodiscard]] outcome fill_slot(std::string_view word, options& opts, std::size_t slot)
{
    auto const unexpected_here = [word]
    { return std::unexpected(std::format("unexpected argument: {}", word)); };

    switch (opts.which)
    {
        case command::show:
            if (slot == 0)
            {
                opts.card = std::string(word);
                return {};
            }
            if (slot == 1)
            {
                opts.target = std::string(word);
                return {};
            }
            return unexpected_here();

        case command::draw:
        {
            if (slot != 0)
                return unexpected_here();

            auto const count = parse_count(word);
            if (!count.has_value())
                return std::unexpected(std::format("draw takes a whole number, not '{}'", word));

            opts.count = *count;
            return {};
        }

        case command::validate:
        case command::list:
        case command::none:
            if (slot != 0)
                return unexpected_here();

            opts.target = std::string(word);
            return {};
    }

    return unexpected_here();
}

// Handle one bare word: the subcommand if we have not seen one, else a slot.
[[nodiscard]] outcome apply_positional(std::string_view word, options& opts, positionals& where)
{
    if (!where.saw_command)
    {
        subcommand const* const which = find_subcommand(word);
        if (which == nullptr)
            return std::unexpected(std::format("unknown subcommand: {}", word));

        opts.which = which->which;
        where.saw_command = true;
        return {};
    }

    return fill_slot(word, opts, where.taken++);
}

// Reject a flag that belongs to a different subcommand.
//
// Global flags go everywhere. Anything else is a usage error rather than a
// silent no-op, which is what makes `show --format json` exit 4 instead of
// quietly rendering text.
[[nodiscard]] outcome check_sections(options const& opts, parse_state const& state)
{
    if (opts.which == command::none)
        return {};

    auto const mine = section_of(opts.which);

    for (auto const& name : state.seen)
    {
        flag const* const found = find_flag(name);
        if (found == nullptr)
            continue;

        if (has_section(found->sections, help_section::global) ||
            has_section(found->sections, mine))
            continue;

        return std::unexpected(std::format("{} is not a flag of `{}`", name, name_of(opts.which)));
    }

    return {};
}

}  // namespace

parse_result parse(std::span<std::string_view const> args)
{
    options opts;
    parse_state state;
    cursor walk(args);
    positionals where;

    while (!walk.done())
    {
        auto const argument = walk.take();

        auto applied = argument.starts_with('-') && argument != "-"
                           ? apply_flag(split(argument), walk, opts, state)
                           : apply_positional(argument, opts, where);

        if (!applied.has_value())
            return std::unexpected(std::move(applied).error());
    }

    if (opts.deck.has_value() && opts.target.has_value())
        return std::unexpected("pass either --deck NAME or a positional TARGET, not both");

    if (auto const sections = check_sections(opts, state); !sections.has_value())
        return std::unexpected(sections.error());

    opts.color_explicit = state.color_set;

    return opts;
}

}  // namespace cartomancer::cli
