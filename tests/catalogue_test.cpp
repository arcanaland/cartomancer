// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "harness.hpp"

#include "cli/text.hpp"
#include "json.hpp"

#include <arcana/validation.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

using namespace cartomancer::testing;  // NOLINT(google-build-using-namespace)

namespace
{

[[nodiscard]] std::size_t line_count(std::string const& text)
{
    return static_cast<std::size_t>(std::ranges::count(text, '\n'));
}

[[nodiscard]] std::string_view line_containing(std::string const& text, std::string_view needle)
{
    auto const at = text.find(needle);
    if (at == std::string::npos)
        return {};

    auto const from = text.rfind('\n', at);
    auto const start = from == std::string::npos ? 0 : from + 1;

    return std::string_view{text}.substr(start, text.find('\n', at) - start);
}

// The first catalogued rule citing a section of this schema major.
[[nodiscard]] arcana::rule const* rule_citing(std::uint8_t major)
{
    auto const found = std::ranges::find_if(
        arcana::rules(),
        [major](arcana::rule const& entry)
        {
            return std::ranges::any_of(
                entry.citations(), [major](arcana::spec_section const& section)
                { return section.schema_major == major; }
            );
        }
    );

    return found == arcana::rules().end() ? nullptr : &*found;
}

}  // namespace

TEST_CASE("the flag --list-codes prints one line per catalogued rule", "[catalogue]")
{
    auto const result = run_cli({"--list-codes"});

    REQUIRE(result.status == 0);

    // One row per rule, plus the column header the aligned table gained.
    REQUIRE(line_count(result.out) == arcana::rules().size() + 1);

    auto const header = result.out.substr(0, result.out.find('\n'));
    REQUIRE(header.starts_with("CODE"));
    REQUIRE(header.ends_with("SCHEMA"));
    for (auto const* column : {"STATE", "LEVEL", "AREA", "NEEDS"}) REQUIRE(header.contains(column));

    REQUIRE_FALSE(result.out.contains('\t'));
}

TEST_CASE("the flag --list-codes reports the whole catalogue", "[catalogue]")
{
    auto const result = run_cli({"--list-codes"});

    for (auto const& entry : arcana::rules()) REQUIRE(result.out.contains(entry.code));
}

TEST_CASE("the flag --list-codes works under validate too", "[catalogue]")
{
    auto const bare = run_cli({"--list-codes"});
    auto const under = run_cli({"validate", "--list-codes"});

    REQUIRE(under.status == 0);
    REQUIRE(under.out == bare.out);
}

TEST_CASE("the flag --list-codes --format json lists every rule", "[catalogue]")
{
    auto const result = run_cli({"--list-codes", "--format", "json"});

    REQUIRE(result.status == 0);
    REQUIRE(result.out.contains("\"rules\""));
    REQUIRE(result.out.contains("\"code\""));
    for (auto const& entry : arcana::rules()) REQUIRE(result.out.contains(entry.code));
}

TEST_CASE("the flag --list-codes reports each rule's implementation state", "[catalogue]")
{
    auto const result = run_cli({"--list-codes"});

    REQUIRE(result.status == 0);

    for (auto const& entry : arcana::rules())
    {
        INFO("rule: " << entry.code);

        auto const state = arcana::state_of(entry.code);
        REQUIRE(state.has_value());

        auto const line = line_containing(result.out, entry.code);
        REQUIRE(line.contains(cartomancer::cli::rule_state_name(*state)));
    }
}

TEST_CASE("the flag --list-codes --format json carries the state", "[catalogue]")
{
    auto const result = run_cli({"--list-codes", "--format", "json"});

    REQUIRE(result.status == 0);
    REQUIRE(result.out.contains("\"state\""));

    for (auto const& entry : arcana::rules())
    {
        INFO("rule: " << entry.code);

        auto const state = arcana::state_of(entry.code);
        REQUIRE(state.has_value());
        REQUIRE(result.out.contains(
            std::string{"\""} + std::string{cartomancer::cli::rule_state_name(*state)} + "\""
        ));
    }
}

TEST_CASE("the flag --explain reports the implementation state", "[catalogue]")
{
    auto const& first = arcana::rules().front();
    auto const state = arcana::state_of(first.code);
    REQUIRE(state.has_value());

    auto const result = run_cli({"--explain", first.code});

    REQUIRE(result.status == 0);
    REQUIRE(result.out.contains("state:"));
    REQUIRE(result.out.contains(cartomancer::cli::rule_state_name(*state)));
}

TEST_CASE("the flag --explain prints one catalogue entry", "[catalogue]")
{
    auto const& first = arcana::rules().front();
    auto const result = run_cli({"--explain", first.code});

    REQUIRE(result.status == 0);
    REQUIRE(result.out.contains(first.code));
    REQUIRE(result.out.contains(first.explanation));
}

TEST_CASE("the flag --explain on an unknown code is a usage error", "[catalogue]")
{
    auto const result = run_cli({"--explain", "no-such-rule"});

    REQUIRE(result.status == 4);
    REQUIRE(result.err.contains("no-such-rule"));
}

TEST_CASE("the flag --explain --format json has the explanation", "[catalogue]")
{
    auto const& first = arcana::rules().front();
    auto const result = run_cli({"--explain", first.code, "--format", "json"});

    REQUIRE(result.status == 0);
    REQUIRE(result.out.contains("\"explanation\""));

    auto const parsed = cartomancer::json::document::parse(result.out);
    REQUIRE(parsed.contains("spec_refs"));
    REQUIRE_FALSE(parsed.contains("spec_ref"));
}

TEST_CASE("the flag --list-codes --format json gives each rule typed spec_refs", "[catalogue]")
{
    auto const result = run_cli({"--list-codes", "--format", "json"});
    REQUIRE(result.status == 0);

    auto const parsed = cartomancer::json::document::parse(result.out);
    auto const& rules = parsed.at("rules");
    REQUIRE(rules.size() == arcana::rules().size());

    for (std::size_t i = 0; i < rules.size(); ++i)
    {
        auto const& entry = arcana::rules()[i];
        auto const& emitted = rules.at(i);
        INFO("rule: " << entry.code);

        REQUIRE_FALSE(emitted.contains("spec_ref"));
        auto const& refs = emitted.at("spec_refs");
        REQUIRE(refs.is_array());
        REQUIRE(refs.size() == entry.citations().size());

        std::size_t at = 0;
        for (auto const& section : entry.citations())
        {
            auto const& ref = refs.at(at++);
            REQUIRE(ref.size() == 3);
            REQUIRE(ref.at("schema_major").is_number_unsigned());
            REQUIRE(ref.at("anchor").is_string());
            REQUIRE(ref.at("url").is_string());

            REQUIRE(ref.at("schema_major").get<unsigned>() == section.schema_major);
            REQUIRE(ref.at("anchor").get<std::string>() == section.anchor);
        }
    }
}

TEST_CASE(
    "the flag --explain --format json links v2 citations at the pinned revision", "[catalogue]"
)
{
    auto const revision = arcana::spec_revision(2);
    REQUIRE_FALSE(revision.empty());

    auto const* entry = rule_citing(2);
    REQUIRE(entry != nullptr);

    auto const result = run_cli({"--explain", entry->code, "--format", "json"});
    REQUIRE(result.status == 0);

    auto const parsed = cartomancer::json::document::parse(result.out);
    auto const& refs = parsed.at("spec_refs");

    bool saw_v2 = false;
    for (auto const& ref : refs)
    {
        if (ref.at("schema_major").get<unsigned>() != 2)
            continue;

        saw_v2 = true;
        REQUIRE(ref.at("url").get<std::string>().contains(revision));
    }
    REQUIRE(saw_v2);
}

TEST_CASE("the flag --explain prints one spec line per citation", "[catalogue]")
{
    // The rule with the most citations exercises the continuation lines.
    auto const* entry = &*std::ranges::max_element(
        arcana::rules(), {}, [](arcana::rule const& rule) { return rule.citations().size(); }
    );
    REQUIRE(entry->citations().size() > 1);

    auto const result = run_cli({"--explain", entry->code});
    REQUIRE(result.status == 0);

    auto const spec = result.out.find("spec:");
    REQUIRE(spec != std::string::npos);

    // Every citation in order, each on its own line, aligned under the first value.
    auto const indent = std::string_view{"spec:         "}.size();
    auto const from = result.out.rfind('\n', spec) + 1;
    auto rest = std::string_view{result.out}.substr(from);

    for (auto const& section : entry->citations())
    {
        auto const line = rest.substr(0, rest.find('\n'));
        INFO("line: " << line);

        auto const url = arcana::spec_url(section);
        auto const expected = url.empty()
                                  ? std::string{"v"} + std::to_string(section.schema_major) + " " +
                                        std::string{section.anchor}
                                  : url;

        REQUIRE(line.size() == indent + expected.size());
        REQUIRE(line.substr(indent) == expected);
        rest.remove_prefix(line.size() + 1);
    }

    REQUIRE(rest.starts_with("schema:"));
}
