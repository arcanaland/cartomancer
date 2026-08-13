// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "catalogue.hpp"

#include "cli/text.hpp"
#include "json.hpp"

#include <arcana/validation.hpp>

#include <format>
#include <ostream>

namespace cartomancer
{

namespace
{

[[nodiscard]] std::string_view phase_name(arcana::phase needs) noexcept
{
    switch (needs)
    {
        case arcana::phase::document:
            return "document";
        case arcana::phase::filesystem:
            return "filesystem";
        case arcana::phase::library:
            return "library";
    }
    return "unknown";
}

[[nodiscard]] std::string applies_to(arcana::rule const& entry)
{
    if (entry.applies_to.min == entry.applies_to.max)
        return std::format("{}", entry.applies_to.min);

    return std::format("{}-{}", entry.applies_to.min, entry.applies_to.max);
}

// The --list-codes column widths, header rows included.
struct columns
{
    std::size_t code = 0;
    std::size_t level = 0;
    std::size_t area = 0;
    std::size_t needs = 0;
};

[[nodiscard]] std::string pad(std::string_view text, std::size_t width)
{
    std::string padded{text};
    padded.append(width - std::min(width, cli::display_width(text)), ' ');
    return padded;
}

[[nodiscard]] json::document rule_json(arcana::rule const& entry)
{
    return json::document{
        {"code", json::from_view(entry.code)},
        {"default_level", json::from_view(cli::severity_name(entry.default_level))},
        {"area", json::from_view(entry.area)},
        {"needs", json::from_view(phase_name(entry.needs))},
        {"spec_ref", json::from_view(entry.spec_ref)},
        {"applies_to", applies_to(entry)},
        {"experimental", entry.experimental},
    };
}

}  // namespace

cli::exit_code run_list_codes(cli::options const& opts, cli::streams sink)
{
    auto const catalogue = arcana::rules();

    if (opts.format == cli::output_format::json)
    {
        auto rules = json::document::array();
        for (auto const& entry : catalogue) rules.push_back(rule_json(entry));

        json::write(sink.out, json::document{{"rules", std::move(rules)}});
        return cli::exit_code::ok;
    }

    auto const& look = sink.style;
    auto const& style = look.style;

    columns widest{
        .code = cli::display_width("CODE"),
        .level = cli::display_width("LEVEL"),
        .area = cli::display_width("AREA"),
        .needs = cli::display_width("NEEDS"),
    };

    for (auto const& entry : catalogue)
    {
        widest.code = std::max(widest.code, cli::display_width(entry.code));
        widest.level =
            std::max(widest.level, cli::display_width(cli::severity_name(entry.default_level)));
        widest.area = std::max(widest.area, cli::display_width(entry.area));
        widest.needs = std::max(widest.needs, cli::display_width(phase_name(entry.needs)));
    }

    sink.out << std::format(
        "{}{}  {}  {}  {}  SCHEMA{}\n", style.muted, pad("CODE", widest.code),
        pad("LEVEL", widest.level), pad("AREA", widest.area), pad("NEEDS", widest.needs),
        style.reset
    );

    for (auto const& entry : catalogue)
    {
        sink.out << std::format(
            "{}{}{}  {}{}{}  {}  {}  {}{}\n", style.accent, pad(entry.code, widest.code),
            style.reset, cli::severity_style(look, entry.default_level),
            pad(cli::severity_name(entry.default_level), widest.level), style.reset,
            pad(entry.area, widest.area), pad(phase_name(entry.needs), widest.needs),
            applies_to(entry),
            entry.experimental ? std::format("  {}experimental{}", style.muted, style.reset)
                               : std::string{}
        );
    }

    return cli::exit_code::ok;
}

cli::exit_code run_explain(cli::options const& opts, std::string_view code, cli::streams sink)
{
    auto const* entry = arcana::find_rule(code);

    if (entry == nullptr)
    {
        sink.err << std::format(
            "{}{}{} no such diagnostic code: {}\n", sink.style.style.error, sink.style.mark.bad,
            sink.style.style.reset, code
        );
        return cli::exit_code::usage;
    }

    if (opts.format == cli::output_format::json)
    {
        auto document = rule_json(*entry);
        document["explanation"] = json::from_view(entry->explanation);

        json::write(sink.out, document);
        return cli::exit_code::ok;
    }

    auto const& look = sink.style;
    auto const& style = look.style;

    auto const field = [&sink, &style](std::string_view label, std::string_view value)
    { sink.out << std::format("{}{}{}{}\n", style.muted, label, style.reset, value); };

    sink.out << std::format(
        "{}{}{} ({})\n\n", cli::severity_style(look, entry->default_level), entry->code,
        style.reset, cli::severity_name(entry->default_level)
    );
    sink.out << std::format("{}\n\n", entry->explanation);

    field("area:         ", entry->area);
    field("needs:        ", phase_name(entry->needs));
    field("spec:         ", entry->spec_ref);
    field("schema:       ", applies_to(*entry));
    if (entry->experimental)
        field("experimental: ", "yes");

    return cli::exit_code::ok;
}

}  // namespace cartomancer
