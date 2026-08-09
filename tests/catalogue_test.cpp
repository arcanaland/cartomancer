// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "harness.hpp"

#include <arcana/validation.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <string>

using namespace cartomancer::testing;  // NOLINT(google-build-using-namespace)

namespace
{

[[nodiscard]] std::size_t line_count(std::string const& text)
{
    return static_cast<std::size_t>(std::ranges::count(text, '\n'));
}

}  // namespace

TEST_CASE("the flag --list-codes prints one line per catalogued rule", "[catalogue]")
{
    // Reconciled against rules().size() rather than a literal 87: the
    // catalogue grows when the spec does, and a hardcoded count is a test that
    // fails for the wrong reason.
    auto const result = run_cli({"--list-codes"});

    REQUIRE(result.status == 0);
    REQUIRE(line_count(result.out) == arcana::rules().size());
}

TEST_CASE(
    "the flag --list-codes reports the whole catalogue, not the implemented subset", "[catalogue]"
)
{
    auto const result = run_cli({"--list-codes"});

    for (auto const& entry : arcana::rules()) REQUIRE(result.out.contains(entry.code));
}

TEST_CASE("the flag --list-codes carries no implementation-state column", "[catalogue]")
{
    // arcana::rule exposes no checked/pending/deferred field, and hardcoding
    // the codes that happen to have a check body turns an honesty feature into
    // a lie. ADR-009 Consequences records the cross-repo ask.
    auto const result = run_cli({"--list-codes"});

    REQUIRE_FALSE(result.out.contains("pending"));
    REQUIRE_FALSE(result.out.contains("implemented"));
    REQUIRE_FALSE(result.out.contains("deferred"));
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

TEST_CASE("the flag --explain --format json carries the explanation", "[catalogue]")
{
    auto const& first = arcana::rules().front();
    auto const result = run_cli({"--explain", first.code, "--format", "json"});

    REQUIRE(result.status == 0);
    REQUIRE(result.out.contains("\"explanation\""));
    REQUIRE(result.out.contains("\"spec_ref\""));
}
