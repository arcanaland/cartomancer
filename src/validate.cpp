// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "validate.hpp"

#include "json.hpp"

#include <algorithm>
#include <array>
#include <expected>
#include <filesystem>
#include <format>
#include <memory>
#include <ostream>
#include <string>

namespace cartomancer
{

namespace
{

namespace fs = std::filesystem;

using loaded_deck = std::expected<std::shared_ptr<arcana::deck const>, arcana::error>;

// A deck the command was asked to act on, plus the spelling of the request we
// echo back as the JSON "target".
struct resolution
{
    std::string target;
    loaded_deck deck;
};

[[nodiscard]] std::string_view error_code_name(arcana::error_code code) noexcept
{
    switch (code)
    {
        case arcana::error_code::not_found:
            return "not_found";
        case arcana::error_code::parse_error:
            return "parse_error";
        case arcana::error_code::io_error:
            return "io_error";
        case arcana::error_code::invalid_argument:
            return "invalid_argument";
    }
    return "unknown";
}

// Look a deck up by its directory name in the library.
[[nodiscard]] resolution resolve_by_name(
    arcana::deck_library const& library, std::string const& name
)
{
    auto const summary = library.find(name);
    if (!summary.has_value())
    {
        // A name carrying a separator was almost certainly meant as a path, so
        // say so rather than report it as a missing library entry.
        auto const message = name.contains(fs::path::preferred_separator)
                                 ? std::format("no deck directory at '{}'", name)
                                 : std::format("no deck named '{}' in the library", name);

        return {
            .target = name,
            .deck = std::unexpected(
                arcana::error{
                    .code = arcana::error_code::not_found,
                    .message = message,
                }
            ),
        };
    }

    return {.target = summary->path.string(), .deck = library.load(name)};
}

// Resolve which deck to act on.
//
// ADR-009's order is --deck NAME, then positional TARGET. A TARGET that exists
// on disk is loaded directly; anything else is a directory name to look up.
// `validate` with neither selector means the current directory.
[[nodiscard]] resolution resolve(arcana::deck_library const& library, options const& opts)
{
    if (opts.deck.has_value())
        return resolve_by_name(library, *opts.deck);

    auto const requested = opts.target.value_or(".");

    std::error_code failed;
    if (fs::is_directory(requested, failed))
    {
        auto absolute = fs::weakly_canonical(requested, failed);
        auto target = failed ? fs::path(requested) : absolute;
        return {.target = target.string(), .deck = library.load_external(requested)};
    }

    return resolve_by_name(library, requested);
}

[[nodiscard]] std::string location_of(arcana::diagnostic const& found)
{
    if (found.card.has_value())
        return *found.card;
    if (found.path.has_value())
        return found.path->string();
    if (found.key.has_value())
        return *found.key;
    return {};
}

// Counts per severity, indexed by the enum's underlying value.
using tally = std::array<std::size_t, 4>;

[[nodiscard]] tally count(std::span<arcana::diagnostic const> reported)
{
    tally counts{};
    for (auto const& found : reported) ++counts.at(static_cast<std::size_t>(found.level));
    return counts;
}

[[nodiscard]] std::size_t tally_of(tally const& counts, arcana::severity level)
{
    return counts.at(static_cast<std::size_t>(level));
}

void write_text(resolution const& what, std::span<arcana::diagnostic const> reported, streams sink)
{
    bool const colored = sink.colored;

    for (auto const& found : reported)
    {
        auto const where = location_of(found);
        sink.out << std::format(
            "{}{}{}: {}: {}{}\n", severity_color(found.level, colored), severity_name(found.level),
            color_reset(colored), found.code, found.message,
            where.empty() ? std::string{} : std::format(" ({})", where)
        );
    }

    auto const counts = count(reported);
    sink.out << std::format(
        "\n{}: {} error(s), {} warning(s), {} info, {} pedantic\n", what.target,
        tally_of(counts, arcana::severity::error), tally_of(counts, arcana::severity::warning),
        tally_of(counts, arcana::severity::info), tally_of(counts, arcana::severity::pedantic)
    );
}

void write_json(
    resolution const& what, arcana::deck const& subject,
    std::span<arcana::diagnostic const> reported, streams sink
)
{
    auto diagnostics = json::document::array();
    for (auto const& found : reported)
    {
        diagnostics.push_back(
            json::document{
                {"level", json::from_view(severity_name(found.level))},
                {"code", json::from_view(found.code)},
                {"message", found.message},
                {"card", found.card},
                {"path", json::from_path(found.path)},
                {"key", found.key},
            }
        );
    }

    // Counts reported diagnostics, i.e. after the --level floor, so it always
    // reconciles with the array beside it. ADR-009.
    auto const counts = count(reported);
    json::document const summary{
        {"error", tally_of(counts, arcana::severity::error)},
        {"warning", tally_of(counts, arcana::severity::warning)},
        {"info", tally_of(counts, arcana::severity::info)},
        {"pedantic", tally_of(counts, arcana::severity::pedantic)},
    };

    json::document const report{
        {"target", what.target},
        {"deck_id", subject.metadata.id},
        {"schema_version", subject.metadata.schema_version},
        {"diagnostics", std::move(diagnostics)},
        {"summary", summary},
    };

    json::write(sink.out, report);
}

// Exit 3's report. ADR-009 fixes the success shape and is silent on this one;
// emitting nothing would leave a --format json consumer parsing an empty
// stream, so we emit a distinguishable object with no "diagnostics" key.
void write_unloadable(resolution const& what, options const& opts, streams sink)
{
    auto const& failure = what.deck.error();

    if (opts.format != output_format::json)
    {
        sink.err << std::format("cannot read deck {}: {}\n", what.target, failure.message);
        return;
    }

    json::document const failed{
        {"code", json::from_view(error_code_name(failure.code))},
        {"message", failure.message},
    };

    json::document const report{
        {"target", what.target},
        {"error", failed},
    };

    json::write(sink.out, report);
}

}  // namespace

std::vector<arcana::diagnostic> apply_floor(
    std::span<arcana::diagnostic const> found, arcana::severity floor
)
{
    std::vector<arcana::diagnostic> reported;
    for (auto const& one : found)
        if (one.level >= floor)
            reported.push_back(one);
    return reported;
}

exit_code code_for(std::span<arcana::diagnostic const> reported) noexcept
{
    bool warned = false;
    for (auto const& found : reported)
    {
        if (found.level == arcana::severity::error)
            return exit_code::errors;
        if (found.level == arcana::severity::warning)
            warned = true;
    }

    // info and pedantic diagnostics are observations, not failures: `info`
    // means nothing is wrong. Only warnings and errors move the exit code.
    return warned ? exit_code::warnings : exit_code::ok;
}

int run_validate(options const& opts, arcana::deck_library const& library, streams sink)
{
    auto const what = resolve(library, opts);

    if (!what.deck.has_value())
    {
        write_unloadable(what, opts, sink);
        return to_int(exit_code::unloadable);
    }

    auto const& subject = **what.deck;
    auto const found = arcana::validate(subject);
    auto const reported = apply_floor(found, opts.level);

    if (opts.format == output_format::json)
        write_json(what, subject, reported, sink);
    else
        write_text(what, reported, sink);

    return to_int(code_for(reported));
}

}  // namespace cartomancer
