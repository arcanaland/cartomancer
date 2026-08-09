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

void write_rule_json(json::writer& out, arcana::rule const& entry)
{
    out.begin_object();
    out.key("code");
    out.string(entry.code);
    out.key("default_level");
    out.string(severity_name(entry.default_level));
    out.key("area");
    out.string(entry.area);
    out.key("needs");
    out.string(phase_name(entry.needs));
    out.key("spec_ref");
    out.string(entry.spec_ref);
    out.key("applies_to");
    out.string(applies_to(entry));
    out.key("experimental");
    out.boolean(entry.experimental);
    out.end_object();
}

}  // namespace

int run_list_codes(options const& opts, streams sink)
{
    auto const catalogue = arcana::rules();

    if (opts.format == output_format::json)
    {
        json::writer out(sink.out);
        out.begin_object();
        out.key("rules");
        out.begin_array();
        for (auto const& entry : catalogue) write_rule_json(out, entry);
        out.end_array();
        out.end_object();
        out.finish();
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
        json::writer out(sink.out);
        out.begin_object();
        out.key("code");
        out.string(entry->code);
        out.key("default_level");
        out.string(severity_name(entry->default_level));
        out.key("area");
        out.string(entry->area);
        out.key("needs");
        out.string(phase_name(entry->needs));
        out.key("spec_ref");
        out.string(entry->spec_ref);
        out.key("applies_to");
        out.string(applies_to(*entry));
        out.key("experimental");
        out.boolean(entry->experimental);
        out.key("explanation");
        out.string(entry->explanation);
        out.end_object();
        out.finish();
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
