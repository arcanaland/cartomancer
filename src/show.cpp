// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "show.hpp"

#include "cli/color.hpp"
#include "cli/text.hpp"
#include "render/art.hpp"
#include "render/escape.hpp"
#include "render/layout.hpp"
#include "target.hpp"

#include <algorithm>
#include <expected>
#include <format>
#include <ostream>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace cartomancer
{

namespace
{

// Rows the terminal keeps for the shell prompt and a little air.
constexpr std::size_t reserved_rows = 4;

// The narrowest panel we will wrap prose into.
constexpr std::size_t prose_floor = 20;

// The panel is a list of already-styled lines. Nothing is padded after it, so
// its styles need no width bookkeeping.
using panel = std::vector<std::string>;

// text broken onto lines of at most limit columns, on spaces.
[[nodiscard]] std::vector<std::string> wrapped(std::string_view text, std::size_t limit)
{
    std::vector<std::string> lines;
    std::string line;

    for (auto const word : std::views::split(text, ' '))
    {
        std::string_view const one{word};
        if (one.empty())
            continue;

        if (line.empty())
            line = one;
        else if (cli::display_width(line) + 1 + cli::display_width(one) <= limit)
            line += std::format(" {}", one);
        else
        {
            lines.push_back(std::move(line));
            line = one;
        }
    }

    if (!line.empty())
        lines.push_back(std::move(line));

    return lines;
}

// `LABEL  value`, the label muted and padded to a common column.
[[nodiscard]] std::string field(
    cli::theme const& look, std::string_view label, std::string_view value
)
{
    constexpr std::size_t label_column = 9;

    std::string row{look.style.muted};
    row += label;
    row += look.style.reset;
    row.append(label_column > label.size() ? label_column - label.size() : 1, ' ');
    row += value;

    return row;
}

[[nodiscard]] panel info_panel(
    arcana::deck const& subject, arcana::card const& one, render::artwork const& art,
    cli::theme const& look, std::size_t width
)
{
    auto const& style = look.style;

    panel rows;

    rows.push_back(std::format("{}{}{}", style.strong, one.display_name, style.reset));
    rows.push_back(std::format("{}{}{}", style.accent, one.canonical_id(), style.reset));
    rows.emplace_back();

    if (!one.id.is_major() && !one.display_rank.empty())
        rows.push_back(
            field(look, "SUIT", std::format("{} of {}", one.display_rank, one.display_suit))
        );

    if (one.number.has_value())
        rows.push_back(field(look, "NUMBER", *one.number));

    if (one.position.has_value())
        rows.push_back(field(look, "POSITION", std::format("{}", *one.position)));

    rows.push_back(
        field(look, "DECK", std::format("{} {}", subject.metadata.name, subject.metadata.version))
    );

    if (art.problem.has_value())
        rows.push_back(field(look, "ARTWORK", std::format("{} (unusable)", art.source)));
    else if (!art.source.empty())
        rows.push_back(field(look, "ARTWORK", art.source));
    else if (one.images.empty())
        rows.push_back(field(look, "ARTWORK", "none"));

    if (one.alt_text.has_value() && !one.alt_text->empty())
    {
        rows.emplace_back();
        for (auto const& line : wrapped(*one.alt_text, std::max(width, prose_floor)))
            rows.push_back(std::format("{}{}{}", style.muted, line, style.reset));
    }

    return rows;
}

// The art rows and the panel rows side by side.
void write_columns(
    render::artwork const& art, panel const& rows, cli::theme const& look, std::ostream& out
)
{
    auto const height = std::max(art.lines.size(), rows.size());

    for (std::size_t row = 0; row < height; ++row)
    {
        std::string line;

        if (art.columns > 0)
        {
            auto const drawn =
                row < art.lines.size() ? std::string_view{art.lines[row]} : std::string_view{};
            line += drawn;

            // Always reset before the padding. The Go tool padded with the
            // art's last background still set and bled it into the panel.
            line += look.style.reset;

            auto const used = render::visible_width(drawn);
            line.append(art.columns - std::min(art.columns, used) + render::gutter, ' ');
        }

        if (row < rows.size())
            line += rows[row];

        // A row of pure padding is trailing whitespace and nothing else.
        while (!line.empty() && line.back() == ' ') line.pop_back();

        out << line << '\n';
    }
}

[[nodiscard]] render::art_size wanted_size(
    arcana::deck const& subject, cli::options const& opts, cli::theme const& look
)
{
    if (opts.no_art || look.depth == cli::color_depth::none)
        return {};

    auto const rows =
        opts.lines.value_or(look.lines > reserved_rows ? look.lines - reserved_rows : look.lines);

    return render::art_dimensions(rows, subject.metadata.aspect_ratio, look.width);
}

[[nodiscard]] cli::exit_code cannot_load(deck_target const& what, cli::streams sink)
{
    cli::write_problem(
        sink, std::format("cannot read deck {}: {}", what.name, what.deck.error().message)
    );
    return cli::exit_code::unloadable;
}

}  // namespace

void write_card(
    arcana::deck const& subject, arcana::card const& one, cli::options const& opts,
    cli::streams sink
)
{
    auto const& look = sink.style;

    auto const art = render::draw_card(one, wanted_size(subject, opts, look), look.depth);

    auto const spent = art.columns > 0 ? art.columns + render::gutter : 0;
    auto const width = look.width > spent ? look.width - spent : 0;

    write_columns(art, info_panel(subject, one, art, look, width), look, sink.out);

    if (art.problem.has_value())
        cli::write_problem(sink, *art.problem);
}

cli::exit_code run_show(
    cli::options const& opts, arcana::deck_library const& library, cli::streams sink
)
{
    if (!opts.card.has_value())
    {
        cli::write_problem(sink, "show needs a card id, e.g. major_arcana.00");
        return cli::exit_code::usage;
    }

    // An unparseable id is a usage error; a well-formed one the deck does not
    // hold is a fact about the deck, and that is exit 3.
    auto const id = arcana::card_id::parse(*opts.card);
    if (!id.has_value())
    {
        cli::write_problem(sink, std::format("not a card id: {}", *opts.card));
        return cli::exit_code::usage;
    }

    auto const what = resolve_deck(library, opts.deck, opts.target);
    if (!what.deck.has_value())
        return cannot_load(what, sink);

    auto const& subject = **what.deck;
    auto const canonical = id->to_canonical();

    auto const one = subject.find_card(*id);
    if (!one.has_value())
    {
        if (auto const excluded = subject.exclusion_reason(canonical))
            cli::write_problem(
                sink,
                excluded->empty()
                    ? std::format("{} excludes {}", subject.metadata.name, canonical)
                    : std::format("{} excludes {}: {}", subject.metadata.name, canonical, *excluded)
            );
        else
            cli::write_problem(
                sink, std::format("{} does not have {}", subject.metadata.name, canonical)
            );

        return cli::exit_code::unloadable;
    }

    write_card(subject, *one, opts, sink);

    return cli::exit_code::ok;
}

}  // namespace cartomancer
