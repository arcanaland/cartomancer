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

// Drop the padding of whatever trailing columns were empty. A deck declaring
// no [deck].id is ordinary, and a line of trailing spaces is not.
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

void write_text(arcana::deck_library const& library, streams sink)
{
    auto const decks = library.decks();
    auto const malformed = library.malformed_decks();

    if (decks.empty() && malformed.empty())
    {
        sink.out << "no decks found in:\n";
        for (auto const& root : library.roots())
            sink.out << std::format("  {}\n", root.string());
        return;
    }

    auto const widest = column_widths(decks);
    for (auto const& summary : decks)
    {
        sink.out << trim_end(std::format(
                        "{}  {}  {:>4} cards  {}",
                        pad(summary.directory_name, widest.directory_name),
                        pad(summary.name, widest.name),
                        summary.card_count,
                        summary.id
                    ))
                 << '\n';
    }

    if (malformed.empty())
        return;

    sink.out << "\nmalformed:\n";
    for (auto const& broken : malformed)
        sink.out << std::format("  {}: {}\n", broken.directory_name, broken.problem.message);
}

void write_json(arcana::deck_library const& library, streams sink)
{
    json::writer out(sink.out);
    out.begin_object();

    out.key("roots");
    out.begin_array();
    for (auto const& root : library.roots())
        out.string(root.string());
    out.end_array();

    out.key("decks");
    out.begin_array();
    for (auto const& summary : library.decks())
    {
        out.begin_object();
        out.key("directory_name");
        out.string(summary.directory_name);
        out.key("path");
        out.string(summary.path.string());
        out.key("id");
        out.string(summary.id);
        out.key("name");
        out.string(summary.name);
        out.key("version");
        out.string(summary.version);
        out.key("author");
        out.string_or_null(summary.author);
        out.key("icon");
        if (summary.icon.has_value())
            out.string(summary.icon->string());
        else
            out.null();
        out.key("card_count");
        out.number(summary.card_count);
        out.end_object();
    }
    out.end_array();

    out.key("malformed");
    out.begin_array();
    for (auto const& broken : library.malformed_decks())
    {
        out.begin_object();
        out.key("directory_name");
        out.string(broken.directory_name);
        out.key("path");
        out.string(broken.path.string());
        out.key("problem");
        out.string(broken.problem.message);
        out.end_object();
    }
    out.end_array();

    out.end_object();
    out.finish();
}

}  // namespace

int run_list(options const& opts, arcana::deck_library const& library, streams sink)
{
    if (opts.format == output_format::json)
        write_json(library, sink);
    else
        write_text(library, sink);

    return to_int(exit_code::ok);
}

}  // namespace cartomancer
