// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "harness.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace cartomancer::testing;  // NOLINT(google-build-using-namespace)

TEST_CASE("list names every readable deck in the roots", "[list]")
{
    auto const result = run_cli({"list"});

    REQUIRE(result.status == 0);
    REQUIRE(result.out.contains("clean-deck"));
    REQUIRE(result.out.contains("warning-deck"));
    REQUIRE(result.out.contains("error-deck"));
}

TEST_CASE("list exits 0 in the presence of a malformed deck", "[list]")
{
    auto const result = run_cli({"list"});

    REQUIRE(result.status == 0);
    REQUIRE(result.out.contains("malformed"));
    REQUIRE(result.out.contains("malformed-deck"));
}

TEST_CASE("list --format json contains the deck_summary field names", "[list]")
{
    auto const result = run_cli({"list", "--format", "json"});

    REQUIRE(result.status == 0);
    REQUIRE(result.out.contains("\"roots\""));
    REQUIRE(result.out.contains("\"decks\""));
    REQUIRE(result.out.contains("\"malformed\""));

    for (auto const* field :
         {"directory_name", "path", "id", "name", "version", "author", "icon", "card_count"})
        REQUIRE(result.out.contains(std::string{'"'} + field + '"'));
}

TEST_CASE("list --format json writes empty optionals as null", "[list]")
{
    auto const result = run_cli({"list", "--format", "json"});

    REQUIRE(result.out.contains("\"author\": null"));
    REQUIRE(result.out.contains("\"icon\": null"));
}

TEST_CASE("list --format json reports malformed decks with a problem", "[list]")
{
    auto const result = run_cli({"list", "--format", "json"});

    REQUIRE(result.status == 0);
    REQUIRE(result.out.contains("\"problem\""));
    REQUIRE(result.out.contains("malformed-deck"));
}

TEST_CASE("list reports the roots it searched", "[list]")
{
    auto const result = run_cli({"list", "--format", "json"});

    REQUIRE(result.out.contains(library_root().string()));
}
