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

// The one row of --list-codes that carries this code.
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

// The honesty property --list-codes exists for: a reader must be able to tell a
// rule the library checks from one it merely catalogues, and the answer comes
// from the library rather than from a list maintained here.
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

    // The vocabulary is the library's, not a spelling invented here: every
    // state the catalogue actually holds is spelled out in the JSON.
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
    REQUIRE(result.out.contains("\"spec_ref\""));
}
