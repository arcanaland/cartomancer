// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "list.hpp"

#include "json.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <ostream>
#include <string>
#include <vector>

namespace cartomancer
{

namespace
{

// Two spaces between every pair of columns.
constexpr std::size_t gap = 2;

// The narrowest a free-text column is allowed to get before the column to its
// right is dropped instead.
constexpr std::size_t floor_width = 6;

// One cell of one row.
struct piece
{
    std::string_view style;
    std::string text;
    std::size_t column = 0;
    bool right = false;
};

// The cells of a row, padded and styled into one line.
[[nodiscard]] std::string compose(std::span<piece const> cells, cli::theme const& look)
{
    std::string row;

    for (std::size_t index = 0; index < cells.size(); ++index)
    {
        auto const& one = cells[index];
        auto const shown = cli::fit(one.text, one.column, look.mark.ellipsis);
        auto const slack = one.column - std::min(one.column, cli::display_width(shown));

        if (one.right)
            row.append(slack, ' ');

        row += one.style;
        row += shown;
        if (!one.style.empty())
            row += look.style.reset;

        if (index + 1 == cells.size())
            break;

        if (!one.right)
            row.append(slack, ' ');

        row.append(gap, ' ');
    }

    return row;
}

// The four text columns.
struct layout
{
    std::size_t deck = 0;
    std::size_t name = 0;
    std::size_t cards = 0;
    std::size_t version = 0;

    // Cleared right to left when even the floor widths will not fit.
    bool show_name = true;
    bool show_cards = true;
    bool show_version = true;

    [[nodiscard]] std::size_t total() const
    {
        auto width = deck;
        if (show_name)
            width += gap + name;
        if (show_cards)
            width += gap + cards;
        if (show_version)
            width += gap + version;
        return width;
    }
};

[[nodiscard]] layout measure(std::span<arcana::deck_summary const> decks)
{
    layout widest{
        .deck = cli::display_width("DECK"),
        .name = cli::display_width("NAME"),
        .cards = cli::display_width("CARDS"),
        .version = cli::display_width("VERSION"),
    };

    for (auto const& summary : decks)
    {
        widest.deck = std::max(widest.deck, cli::display_width(summary.directory_name));
        widest.name = std::max(widest.name, cli::display_width(summary.name));
        widest.cards = std::max(widest.cards, std::format("{}", summary.card_count).size());
        widest.version = std::max(widest.version, cli::display_width(summary.version));
    }

    return widest;
}

// Make the table fit `width`.
[[nodiscard]] layout fit_to(layout widest, std::size_t width)
{
    if (width == 0 || widest.total() <= width)
        return widest;

    auto const floored = [](layout one)
    {
        one.deck = std::min(one.deck, floor_width);
        one.name = std::min(one.name, floor_width);
        return one.total();
    };

    for (bool* shown : {&widest.show_version, &widest.show_cards, &widest.show_name})
    {
        if (floored(widest) <= width)
            break;
        *shown = false;
    }

    // Then take from whichever free-text column is currently the widest.
    while (widest.total() > width &&
           ((widest.show_name && widest.name > floor_width) || widest.deck > floor_width))
    {
        if (widest.show_name && widest.name > floor_width && widest.name >= widest.deck)
            --widest.name;
        else
            --widest.deck;
    }

    // A terminal too narrow even for that
    if (widest.total() > width)
        widest.deck -= std::min(widest.deck, widest.total() - width);

    return widest;
}

// `~/.local/share/tarot/decks`, when $HOME says so.
[[nodiscard]] std::string contract_home(std::filesystem::path const& path)
{
    char const* const home = std::getenv("HOME");
    auto text = path.string();

    if (home == nullptr || *home == '\0')
        return text;

    std::string_view const prefix{home};
    if (!text.starts_with(prefix))
        return text;

    return "~" + text.substr(prefix.size());
}

[[nodiscard]] std::string preamble(arcana::deck_library const& library, std::size_t shown)
{
    auto const roots = library.roots();
    auto const* const decks = shown == 1 ? "deck" : "decks";

    if (roots.size() == 1)
        return std::format("Showing {} {} in {}", shown, decks, contract_home(roots.front()));

    return std::format("Showing {} {} in {} roots", shown, decks, roots.size());
}

// One whole-width line in one style, truncated to the terminal.
void write_line(cli::streams sink, std::string_view text, std::string_view style_of)
{
    auto const& look = sink.style;

    piece const only{
        .style = style_of,
        .text = std::string{text},
        .column = look.width == 0 ? cli::display_width(text) : look.width,
    };

    sink.out << compose({&only, 1}, look) << '\n';
}

// The header row and one row per deck
void write_table(std::span<arcana::deck_summary const> decks, cli::streams sink)
{
    auto const& look = sink.style;
    auto const& style = look.style;
    auto const columns = fit_to(measure(decks), look.width);

    std::vector<piece> header;
    header.push_back({.style = style.muted, .text = "DECK", .column = columns.deck});
    if (columns.show_name)
        header.push_back({.style = style.muted, .text = "NAME", .column = columns.name});
    if (columns.show_cards)
        header.push_back(
            {.style = style.muted, .text = "CARDS", .column = columns.cards, .right = true}
        );
    if (columns.show_version)
        header.push_back({.style = style.muted, .text = "VERSION", .column = columns.version});

    sink.out << compose(header, look) << '\n';

    for (auto const& summary : decks)
    {
        std::vector<piece> row;
        row.push_back(
            {.style = style.accent, .text = summary.directory_name, .column = columns.deck}
        );
        if (columns.show_name)
            row.push_back({.text = summary.name, .column = columns.name});
        if (columns.show_cards)
            row.push_back(
                {.text = std::format("{}", summary.card_count),
                 .column = columns.cards,
                 .right = true}
            );
        if (columns.show_version)
            row.push_back(
                {.style = style.muted, .text = summary.version, .column = columns.version}
            );

        sink.out << compose(row, look) << '\n';
    }
}

void write_malformed(std::span<arcana::malformed_deck const> broken_decks, cli::streams sink)
{
    auto const& look = sink.style;

    sink.out << '\n';
    write_line(sink, std::format("{} malformed", broken_decks.size()), look.style.strong);

    for (auto const& broken : broken_decks)
        write_line(
            sink,
            std::format(
                "  {} {}: {}", look.mark.bad, broken.directory_name, broken.problem.message
            ),
            look.style.error
        );
}

void write_text(arcana::deck_library const& library, cli::streams sink)
{
    auto const& muted = sink.style.style.muted;

    auto const decks = library.decks();
    auto const malformed = library.malformed_decks();

    if (decks.empty() && malformed.empty())
    {
        write_line(sink, "no decks found in:", muted);
        for (auto const& root : library.roots())
            write_line(sink, std::format("  {}", root.string()), muted);
        return;
    }

    if (!decks.empty())
    {
        write_line(sink, preamble(library, decks.size()), muted);
        sink.out << '\n';
        write_table(decks, sink);
    }

    if (!malformed.empty())
        write_malformed(malformed, sink);
}

void write_json(arcana::deck_library const& library, cli::streams sink)
{
    auto roots = json::document::array();
    for (auto const& root : library.roots()) roots.push_back(json::from_path(root));

    auto decks = json::document::array();
    for (auto const& summary : library.decks())
    {
        decks.push_back(
            json::document{
                {"directory_name", summary.directory_name},
                {"path", json::from_path(summary.path)},
                {"identifier", summary.identifier},
                {"name", summary.name},
                {"version", summary.version},
                {"artist", summary.artist},
                {"icon", json::from_path(summary.icon)},
                {"card_count", summary.card_count},
            }
        );
    }

    auto malformed = json::document::array();
    for (auto const& broken : library.malformed_decks())
    {
        malformed.push_back(
            json::document{
                {"directory_name", broken.directory_name},
                {"path", json::from_path(broken.path)},
                {"problem", broken.problem.message},
            }
        );
    }

    json::document const report{
        {"roots", std::move(roots)},
        {"decks", std::move(decks)},
        {"malformed", std::move(malformed)},
    };

    json::write(sink.out, report);
}

}  // namespace

cli::exit_code run_list(
    cli::options const& opts, arcana::deck_library const& library, cli::streams sink
)
{
    if (opts.format == cli::output_format::json)
        write_json(library, sink);
    else
        write_text(library, sink);

    return cli::exit_code::ok;
}

}  // namespace cartomancer
