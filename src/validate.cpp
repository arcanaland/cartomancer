// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "validate.hpp"

#include "cli/color.hpp"
#include "cli/text.hpp"
#include "json.hpp"

#include <algorithm>
#include <expected>
#include <filesystem>
#include <format>
#include <memory>
#include <ostream>
#include <string>
#include <string_view>

namespace cartomancer
{

namespace
{

namespace fs = std::filesystem;

using loaded_deck = std::expected<std::shared_ptr<arcana::deck const>, arcana::error>;

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

// Look a deck by its directory name in the library.
[[nodiscard]] resolution resolve_by_name(
    arcana::deck_library const& library, std::string const& name
)
{
    auto const summary = library.find(name);
    if (!summary.has_value())
    {
        // A name with a separator was almost certainly meant as a path
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
[[nodiscard]] resolution resolve(arcana::deck_library const& library, cli::options const& opts)
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

// Counts per severity.
struct tally
{
    std::size_t error = 0;
    std::size_t warning = 0;
    std::size_t info = 0;
    std::size_t pedantic = 0;
};

[[nodiscard]] tally count(std::span<arcana::diagnostic const> reported)
{
    tally counts;
    for (auto const& found : reported)
    {
        switch (found.level)
        {
            case arcana::severity::error:
                ++counts.error;
                break;
            case arcana::severity::warning:
                ++counts.warning;
                break;
            case arcana::severity::info:
                ++counts.info;
                break;
            case arcana::severity::pedantic:
                ++counts.pedantic;
                break;
        }
    }
    return counts;
}

[[nodiscard]] std::string describe(tally const& counts)
{
    std::string out;

    auto const add = [&out](std::size_t many, std::string_view singular, std::string_view plural)
    {
        if (many == 0)
            return;

        if (!out.empty())
            out += ", ";

        out += std::format("{} {}", many, many == 1 ? singular : plural);
    };

    add(counts.error, "error", "errors");
    add(counts.warning, "warning", "warnings");
    add(counts.info, "info", "info");
    add(counts.pedantic, "pedantic", "pedantic");

    return out;
}

void write_text(
    std::string_view target, arcana::deck const& subject,
    std::span<arcana::diagnostic const> reported, cli::streams sink
)
{
    auto const& look = sink.style;
    auto const& style = look.style;

    // The deck's own name when it has one
    std::string_view const named =
        subject.metadata.name.empty() ? target : std::string_view{subject.metadata.name};

    if (reported.empty())
    {
        sink.out << std::format(
            "{}{}{} {}{}{} {} no problems found\n", style.success, look.mark.ok, style.reset,
            style.strong, named, style.reset, look.mark.dash
        );
        return;
    }

    for (auto const& found : reported)
    {
        sink.out << std::format(
            "{}{}[{}]{}: {}\n", cli::severity_style(look, found.level),
            cli::severity_name(found.level), found.code, style.reset, found.message
        );

        // A diagnostic with nowhere to point so skip location line
        if (auto const where = location_of(found); !where.empty())
            sink.out << std::format(
                "  {}{} {}{}\n", style.muted, look.mark.arrow, where, style.reset
            );
    }

    auto const counts = count(reported);

    auto mark = look.mark.ok;
    auto worst = style.success;

    if (counts.error > 0)
    {
        mark = look.mark.bad;
        worst = style.error;
    }
    else if (counts.warning > 0)
    {
        mark = look.mark.warn;
        worst = style.warning;
    }

    sink.out << std::format(
        "\n{}{}{} {} in {}{}{}\n", worst, mark, style.reset, describe(counts), style.strong, named,
        style.reset
    );
}

void write_json(
    std::string_view target, arcana::deck const& subject,
    std::span<arcana::diagnostic const> reported, cli::streams sink
)
{
    auto diagnostics = json::document::array();
    for (auto const& found : reported)
    {
        diagnostics.push_back(
            json::document{
                {"level", json::from_view(cli::severity_name(found.level))},
                {"code", json::from_view(found.code)},
                {"message", found.message},
                {"card", found.card},
                {"path", json::from_path(found.path)},
                {"key", found.key},
            }
        );
    }

    auto const counts = count(reported);
    json::document const summary{
        {"error", counts.error},
        {"warning", counts.warning},
        {"info", counts.info},
        {"pedantic", counts.pedantic},
    };

    json::document const report{
        {"target", json::from_view(target)},
        {"deck_id", subject.metadata.id},
        {"schema_version", subject.metadata.schema_version},
        {"diagnostics", std::move(diagnostics)},
        {"summary", summary},
    };

    json::write(sink.out, report);
}

void write_unloadable(
    std::string_view target, arcana::error const& failure, cli::options const& opts,
    cli::streams sink
)
{
    if (opts.format != cli::output_format::json)
    {
        sink.err << std::format(
            "{}{}{} cannot read deck {}: {}\n", sink.style.style.error, sink.style.mark.bad,
            sink.style.style.reset, target, failure.message
        );
        return;
    }

    json::document const failed{
        {"code", json::from_view(error_code_name(failure.code))},
        {"message", failure.message},
    };

    json::document const report{
        {"target", json::from_view(target)},
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

cli::exit_code code_for(std::span<arcana::diagnostic const> reported) noexcept
{
    bool warned = false;
    for (auto const& found : reported)
    {
        if (found.level == arcana::severity::error)
            return cli::exit_code::errors;
        if (found.level == arcana::severity::warning)
            warned = true;
    }

    return warned ? cli::exit_code::warnings : cli::exit_code::ok;
}

cli::exit_code run_validate(
    cli::options const& opts, arcana::deck_library const& library, cli::streams sink
)
{
    auto const what = resolve(library, opts);

    if (!what.deck.has_value())
    {
        write_unloadable(what.target, what.deck.error(), opts, sink);
        return cli::exit_code::unloadable;
    }

    auto const& subject = **what.deck;
    auto const found = arcana::validate(subject);
    auto const reported = apply_floor(found, opts.level);

    if (opts.format == cli::output_format::json)
        write_json(what.target, subject, reported, sink);
    else
        write_text(what.target, subject, reported, sink);

    return code_for(reported);
}

}  // namespace cartomancer
