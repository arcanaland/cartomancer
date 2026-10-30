// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "harness.hpp"

#include "cli/text.hpp"

#include <arcana/validation.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
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
    REQUIRE(result.out.contains("\"spec_refs\""));
}

TEST_CASE("the flag --explain cites every section as a pinned URL", "[catalogue]")
{
    // A rule that cites at least two sections, so the continuation lines are
    // exercised too. Every catalogue entry carries citations; take the first
    // multi-section one rather than assuming which.
    auto const& catalogue = arcana::rules();
    auto const found =
        std::ranges::find_if(catalogue, [](auto const& rule) { return rule.spec_refs.size() > 1; });
    REQUIRE(found != catalogue.end());

    auto const result = run_cli({"--explain", found->code});

    REQUIRE(result.status == 0);

    for (auto const& section : found->spec_refs)
    {
        auto const url = arcana::spec_url(section);
        REQUIRE_FALSE(url.empty());
        REQUIRE(result.out.contains(url));
    }

    // Only the first line carries the label; the rest are aligned under it.
    REQUIRE(
        line_containing(result.out, arcana::spec_url(found->spec_refs.front())).starts_with("spec:")
    );
    REQUIRE(
        line_containing(result.out, arcana::spec_url(found->spec_refs.back())).starts_with("  ")
    );
}

TEST_CASE("the flag --explain --format json carries anchors and their URLs", "[catalogue]")
{
    auto const& first = arcana::rules().front();
    REQUIRE_FALSE(first.spec_refs.empty());

    auto const result = run_cli({"--explain", first.code, "--format", "json"});

    REQUIRE(result.status == 0);
    REQUIRE(result.out.contains("\"anchor\""));
    REQUIRE(result.out.contains(first.spec_refs.front().anchor));
    REQUIRE(result.out.contains(arcana::spec_url(first.spec_refs.front())));
}
