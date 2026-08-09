// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "list.hpp"

#include "json.hpp"

#include <algorithm>
#include <format>
#include <ostream>
#include <string>

namespace cartomancer
{

namespace
{

struct widths
{
    std::size_t directory_name = 0;
    std::size_t name = 0;
};

[[nodiscard]] widths column_widths(std::span<arcana::deck_summary const> decks)
{
    widths widest;
    for (auto const& summary : decks)
    {
        widest.directory_name = std::max(widest.directory_name, summary.directory_name.size());
        widest.name = std::max(widest.name, summary.name.size());
    }
    return widest;
}

[[nodiscard]] std::string trim_end(std::string line)
{
    auto const last = line.find_last_not_of(' ');
    line.erase(last == std::string::npos ? 0 : last + 1);
    return line;
}

[[nodiscard]] std::string pad(std::string_view text, std::size_t width)
{
    std::string padded(text);
    if (padded.size() < width)
        padded.append(width - padded.size(), ' ');
    return padded;
}

void write_text(arcana::deck_library const& library, cli::streams sink)
{
    auto const decks = library.decks();
    auto const malformed = library.malformed_decks();

    if (decks.empty() && malformed.empty())
    {
        sink.out << "no decks found in:\n";
        for (auto const& root : library.roots()) sink.out << std::format("  {}\n", root.string());
        return;
    }

    auto const widest = column_widths(decks);
    for (auto const& summary : decks)
    {
        sink.out << trim_end(
                        std::format(
                            "{}  {}  {:>4} cards  {}",
                            pad(summary.directory_name, widest.directory_name),
                            pad(summary.name, widest.name), summary.card_count, summary.id
                        )
                    )
                 << '\n';
    }

    if (malformed.empty())
        return;

    sink.out << "\nmalformed:\n";
    for (auto const& broken : malformed)
        sink.out << std::format("  {}: {}\n", broken.directory_name, broken.problem.message);
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
                {"id", summary.id},
                {"name", summary.name},
                {"version", summary.version},
                {"author", summary.author},
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
