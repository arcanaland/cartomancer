// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "catalogue.hpp"

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

// `explain` emits this and appends "explanation"; --list-codes emits it alone.
// Keep them one function so the shared fields cannot drift apart.
[[nodiscard]] json::document rule_json(arcana::rule const& entry)
{
    return json::document{
        {"code", json::from_view(entry.code)},
        {"default_level", json::from_view(severity_name(entry.default_level))},
        {"area", json::from_view(entry.area)},
        {"needs", json::from_view(phase_name(entry.needs))},
        {"spec_ref", json::from_view(entry.spec_ref)},
        {"applies_to", applies_to(entry)},
        {"experimental", entry.experimental},
    };
}

}  // namespace

int run_list_codes(options const& opts, streams sink)
{
    auto const catalogue = arcana::rules();

    if (opts.format == output_format::json)
    {
        auto rules = json::document::array();
        for (auto const& entry : catalogue) rules.push_back(rule_json(entry));

        json::write(sink.out, json::document{{"rules", std::move(rules)}});
        return to_int(exit_code::ok);
    }

    // One rule per line, so `--list-codes | wc -l` reconciles with
    // rules().size() -- which is what the acceptance criterion checks, rather
    // than a literal 87 that goes stale when the spec grows.
    for (auto const& entry : catalogue)
    {
        sink.out << std::format(
            "{}\t{}\t{}\t{}\t{}{}\n", entry.code, severity_name(entry.default_level), entry.area,
            phase_name(entry.needs), applies_to(entry), entry.experimental ? "\texperimental" : ""
        );
    }

    return to_int(exit_code::ok);
}

int run_explain(options const& opts, std::string_view code, streams sink)
{
    auto const* entry = arcana::find_rule(code);

    if (entry == nullptr)
    {
        sink.err << std::format("no such diagnostic code: {}\n", code);
        return to_int(exit_code::usage);
    }

    if (opts.format == output_format::json)
    {
        auto document = rule_json(*entry);
        document["explanation"] = json::from_view(entry->explanation);

        json::write(sink.out, document);
        return to_int(exit_code::ok);
    }

    sink.out << std::format("{} ({})\n\n", entry->code, severity_name(entry->default_level));
    sink.out << std::format("{}\n\n", entry->explanation);
    sink.out << std::format("area:         {}\n", entry->area);
    sink.out << std::format("needs:        {}\n", phase_name(entry->needs));
    sink.out << std::format("spec:         {}\n", entry->spec_ref);
    sink.out << std::format("schema:       {}\n", applies_to(*entry));
    if (entry->experimental)
        sink.out << "experimental: yes\n";

    return to_int(exit_code::ok);
}

}  // namespace cartomancer
