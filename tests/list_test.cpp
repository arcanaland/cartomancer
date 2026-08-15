// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "harness.hpp"

#include <catch2/catch_test_macros.hpp>

#include <ranges>
#include <string>
#include <string_view>
#include <vector>

using namespace cartomancer;           // NOLINT(google-build-using-namespace)
using namespace cartomancer::testing;  // NOLINT(google-build-using-namespace)

namespace
{

[[nodiscard]] std::vector<std::string> lines_of(std::string const& text)
{
    std::vector<std::string> out;
    for (auto const one : std::views::split(text, '\n'))
    {
        std::string_view const line{one};
        if (!line.empty())
            out.emplace_back(line);
    }
    return out;
}

[[nodiscard]] cli::theme bounded(std::size_t width)
{
    cli::theme narrow;
    narrow.width = width;
    return narrow;
}

}  // namespace

TEST_CASE("list names every readable deck in the roots", "[list]")
{
    auto const result = run_cli({"list"});

    REQUIRE(result.status == 0);
    REQUIRE(result.out.contains("clean-deck"));
    REQUIRE(result.out.contains("warning-deck"));
    REQUIRE(result.out.contains("error-deck"));
}

TEST_CASE("list heads its table and says where it looked", "[list]")
{
    auto const result = run_cli({"list"});
    auto const lines = lines_of(result.out);

    REQUIRE(lines.at(0).starts_with("Showing 3 decks in "));
    REQUIRE(lines.at(1).starts_with("DECK"));
    REQUIRE(lines.at(1).contains("NAME"));
    REQUIRE(lines.at(1).contains("CARDS"));
    REQUIRE(lines.at(1).ends_with("VERSION"));
}


TEST_CASE("no row exceeds a bounded width, and truncation is marked", "[list]")
{
    auto const result = run_cli({"list"}, bounded(28));

    REQUIRE(result.status == 0);
    for (auto const& line : lines_of(result.out)) REQUIRE(cli::display_width(line) <= 28);

    REQUIRE(result.out.contains("..."));
}

TEST_CASE("an unbounded width truncates nothing", "[list]")
{
    auto const result = run_cli({"list"});

    REQUIRE_FALSE(result.out.contains("..."));
    REQUIRE(result.out.contains("Clean Deck"));
    REQUIRE(result.out.contains("Warning Deck"));
}

TEST_CASE("a malformed deck is called out but does not change the exit code", "[list]")
{
    auto const result = run_cli({"list"});

    REQUIRE(result.status == 0);
    REQUIRE(result.out.contains("1 malformed"));
    REQUIRE(result.out.contains("[x] malformed-deck:"));
}

TEST_CASE("no line of the table carries trailing whitespace", "[list]")
{
    auto const result = run_cli({"list"}, styled(cli::color_depth::truecolor));

    for (auto const& line : lines_of(result.out)) REQUIRE_FALSE(line.ends_with(' '));
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
         {"directory_name", "path", "id", "name", "version", "artist", "icon", "card_count"})
        REQUIRE(result.out.contains(std::string{'"'} + field + '"'));
}

TEST_CASE("list --format json writes empty optionals as null", "[list]")
{
    auto const result = run_cli({"list", "--format", "json"});

    REQUIRE(result.out.contains("\"artist\": null"));
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
