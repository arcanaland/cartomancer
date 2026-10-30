// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "cli/color.hpp"
#include "harness.hpp"
#include "render/escape.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstddef>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

using namespace cartomancer;           // NOLINT(google-build-using-namespace)
using namespace cartomancer::testing;  // NOLINT(google-build-using-namespace)

namespace
{

[[nodiscard]] cli::theme drawing(cli::color_depth depth, std::size_t width, std::size_t lines)
{
    return {
        .depth = depth,
        .style = cli::for_depth(depth),
        .mark = cli::unicode_marks,
        .width = width,
        .lines = lines,
    };
}

[[nodiscard]] std::vector<std::string> lines_of(std::string const& text)
{
    std::vector<std::string> out;
    for (auto const one : std::views::split(text, '\n')) out.emplace_back(std::string_view{one});

    if (!out.empty() && out.back().empty())
        out.pop_back();

    return out;
}

// The half block, spelled once.
constexpr std::string_view block = "▀";

}  // namespace

// ---------------------------------------------------------------- show

TEST_CASE("show prints a card's panel without art", "[show]")
{
    auto const result = run_art_cli({"show", "major_arcana.00", "--no-art"});

    REQUIRE(result.status == 0);
    REQUIRE(result.out.contains("major_arcana.00"));
    REQUIRE(result.out.contains("Art Deck"));
}

TEST_CASE("show finds its deck through --deck and through TARGET", "[show]")
{
    REQUIRE(run_art_cli({"show", "major_arcana.00", "--deck", "art-deck", "--no-art"}).status == 0);
    REQUIRE(run_art_cli({"show", "major_arcana.00", "art-deck", "--no-art"}).status == 0);
}

TEST_CASE("show falls back to the reference deck", "[show]")
{
    // No --deck and no TARGET: ADR-009's order ends at reference(), not at ".".
    REQUIRE(run_art_cli({"show", "major_arcana.00", "--no-art"}).status == 0);
}

TEST_CASE("a card id the deck does not hold exits 3", "[show]")
{
    auto const result = run_art_cli({"show", "major_arcana.99", "--no-art"});

    REQUIRE(result.status == 3);
    REQUIRE(result.err.contains("does not have major_arcana.99"));
}

TEST_CASE("an excluded card exits 3 and says why", "[show]")
{
    auto const result = run_art_cli({"show", "major_arcana.21", "--no-art"});

    REQUIRE(result.status == 3);
    REQUIRE(result.err.contains("excludes major_arcana.21"));
    REQUIRE(result.err.contains("on purpose"));
}

TEST_CASE("an unparseable card id is a usage error, not a missing card", "[show]")
{
    auto const result = run_art_cli({"show", "the-fool", "--no-art"});

    REQUIRE(result.status == 4);
    REQUIRE(result.err.contains("not a card id"));
}

TEST_CASE("show without a card id is a usage error", "[show]")
{
    REQUIRE(run_art_cli({"show"}).status == 4);
}

TEST_CASE("show rejects both --deck and TARGET", "[show]")
{
    auto const result = run_art_cli({"show", "major_arcana.00", "art-deck", "--deck", "art-deck"});

    REQUIRE(result.status == 4);
    REQUIRE(result.err.contains("not both"));
}

TEST_CASE("show does not take --format", "[show]")
{
    auto const result = run_art_cli({"show", "major_arcana.00", "--format", "json"});

    REQUIRE(result.status == 4);
    REQUIRE(result.err.contains("--format is not a flag of `show`"));
}

TEST_CASE("a deck that cannot be read exits 3", "[show]")
{
    auto const result = run_art_cli({"show", "major_arcana.00", "--deck", "absent"});

    REQUIRE(result.status == 3);
    REQUIRE(result.err.contains("no deck named 'absent'"));
}

TEST_CASE("at no color depth show prints the panel and no escapes", "[show]")
{
    // A pipe and NO_COLOR both land here. A grid of uncolored half-blocks was
    // the Go tool's defect 5.
    auto const result =
        run_art_cli({"show", "major_arcana.00"}, drawing(cli::color_depth::none, 0, 24));

    REQUIRE(result.status == 0);
    REQUIRE(result.out.find(cli::sgr_escape) == std::string::npos);
    REQUIRE_FALSE(result.out.contains(block));
}

TEST_CASE("a color depth restores the art", "[show]")
{
    auto const result =
        run_art_cli({"show", "major_arcana.00"}, drawing(cli::color_depth::truecolor, 80, 24));

    REQUIRE(result.status == 0);
    REQUIRE(result.out.contains(block));
}

TEST_CASE("the info column starts at the same place on every art row", "[show]")
{
    auto const result =
        run_art_cli({"show", "major_arcana.00"}, drawing(cli::color_depth::truecolor, 80, 12));

    std::vector<std::size_t> starts;
    for (auto const& row : lines_of(result.out))
    {
        auto const plain = render::strip_escapes(row);
        auto const at = plain.find_first_not_of(' ', plain.rfind(block) + block.size());
        if (!plain.contains(block) || at == std::string::npos)
            continue;

        starts.push_back(cli::display_width(std::string_view{plain}.substr(0, at)));
    }

    REQUIRE(starts.size() >= 2);
    REQUIRE(std::ranges::adjacent_find(starts, std::ranges::not_equal_to{}) == starts.end());
}

TEST_CASE("the gutter is reset before it is padded", "[show]")
{
    auto const result =
        run_art_cli({"show", "major_arcana.00"}, drawing(cli::color_depth::truecolor, 80, 12));

    auto const reset = std::string{cli::sgr_escape} + "[0m";

    for (auto const& row : lines_of(result.out))
    {
        if (!row.contains(block))
            continue;

        // Whatever precedes the gutter, a reset is the last thing in it.
        auto const art_ends = row.rfind(block) + block.size();
        REQUIRE(row.substr(art_ends).starts_with(reset));
    }
}

TEST_CASE("the deck's own ANSI art wins over quantizing a raster", "[show]")
{
    // major_arcana.01 ships only ansi4/, and its second line carries a non-SGR
    // sequence that the Go scanner would have eaten.
    auto const result =
        run_art_cli({"show", "major_arcana.01"}, drawing(cli::color_depth::truecolor, 80, 8));

    REQUIRE(result.status == 0);
    REQUIRE(result.out.contains("ansi4"));
    REQUIRE(result.out.contains("##"));
    REQUIRE_FALSE(result.out.contains(block));
}

TEST_CASE("the flag --lines sets the art height", "[show]")
{
    auto const look = drawing(cli::color_depth::truecolor, 100, 24);

    auto const tall = run_art_cli({"show", "major_arcana.00", "--lines", "10"}, look);
    auto const squat = run_art_cli({"show", "major_arcana.00", "--lines", "4"}, look);

    REQUIRE(tall.status == 0);
    REQUIRE(squat.status == 0);
    REQUIRE(lines_of(tall.out).size() > lines_of(squat.out).size());
}

TEST_CASE("the flag --lines wants a positive whole number", "[show]")
{
    REQUIRE(run_art_cli({"show", "major_arcana.00", "--lines", "0"}).status == 4);
    REQUIRE(run_art_cli({"show", "major_arcana.00", "--lines", "tall"}).status == 4);
}

// ---------------------------------------------------------------- draw

TEST_CASE("draw prints one card by default", "[draw]")
{
    auto const result = run_art_cli({"draw"}, drawing(cli::color_depth::none, 0, 24));

    REQUIRE(result.status == 0);
    REQUIRE(result.out.contains("Art Deck"));
}

TEST_CASE("the same seed draws the same cards", "[draw]")
{
    auto const first = run_art_cli({"draw", "3", "--seed", "7"});
    auto const again = run_art_cli({"draw", "3", "--seed", "7"});
    auto const other = run_art_cli({"draw", "3", "--seed", "8"});

    REQUIRE(first.status == 0);
    REQUIRE(first.out == again.out);
    REQUIRE(first.out != other.out);
}

TEST_CASE("a draw never repeats a card", "[draw]")
{
    auto const result = run_art_cli({"draw", "10", "--seed", "1"});

    REQUIRE(result.status == 0);

    std::vector<std::string> ids;
    for (auto const& row : lines_of(result.out))
    {
        auto const plain = render::strip_escapes(row);
        if (plain.starts_with("major_arcana.") || plain.starts_with("minor_arcana."))
            ids.push_back(plain);
    }

    REQUIRE(ids.size() == 10);

    std::ranges::sort(ids);
    REQUIRE(std::ranges::unique(ids).empty());
}

TEST_CASE("drawing more cards than the deck holds is a usage error", "[draw]")
{
    auto const result = run_art_cli({"draw", "500"});

    REQUIRE(result.status == 4);
    REQUIRE(result.err.contains("cannot draw 500 cards"));
}

TEST_CASE("drawing nothing prints nothing and succeeds", "[draw]")
{
    auto const result = run_art_cli({"draw", "0"});

    REQUIRE(result.status == 0);
    REQUIRE(result.out.empty());
}

TEST_CASE("draw's positional is a count, not a deck", "[draw]")
{
    auto const result = run_art_cli({"draw", "art-deck"});

    REQUIRE(result.status == 4);
    REQUIRE(result.err.contains("whole number"));
}

TEST_CASE("draw takes --deck and --seed and nothing of show's", "[draw]")
{
    REQUIRE(run_art_cli({"draw", "1", "--deck", "art-deck", "--seed", "3"}).status == 0);

    auto const borrowed = run_art_cli({"draw", "1", "--deck", "art-deck", "--no-art"});

    REQUIRE(borrowed.status == 4);
    REQUIRE(borrowed.err.contains("--no-art is not a flag of `draw`"));
}

TEST_CASE("the flag --seed wants a whole number", "[draw]")
{
    REQUIRE(run_art_cli({"draw", "1", "--seed", "lucky"}).status == 4);
}

// ---------------------------------------------------------------- surface

TEST_CASE("the flag --help documents all four commands", "[cli]")
{
    auto const result = run_cli({"--help"});

    REQUIRE(result.status == 0);
    REQUIRE(result.out.contains("show"));
    REQUIRE(result.out.contains("draw"));
    REQUIRE(result.out.contains("--lines"));
    REQUIRE(result.out.contains("--no-art"));
    REQUIRE(result.out.contains("--seed"));
}
