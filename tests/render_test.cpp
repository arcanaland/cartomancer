// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "cli/color.hpp"
#include "harness.hpp"
#include "render/art.hpp"
#include "render/escape.hpp"
#include "render/image.hpp"
#include "render/layout.hpp"
#include "render/quantize.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

using namespace cartomancer;           // NOLINT(google-build-using-namespace)
using namespace cartomancer::testing;  // NOLINT(google-build-using-namespace)

namespace
{

// An escape spelled without ever writing the two bytes `just lint-escapes`
// hunts for; the tests are outside src/ but there is no reason to be cute.
[[nodiscard]] std::string csi(std::string_view body)
{
    return std::string{cli::sgr_escape} + "[" + std::string{body};
}

[[nodiscard]] render::raster solid(int width, int height, cli::rgb color)
{
    render::raster one{.width = width, .height = height, .pixels = {}};
    for (int index = 0; index < width * height; ++index)
    {
        one.pixels.push_back(color.red);
        one.pixels.push_back(color.green);
        one.pixels.push_back(color.blue);
        one.pixels.push_back(255);
    }
    return one;
}

}  // namespace

// ---------------------------------------------------------------- escapes

TEST_CASE("the scanner consumes a whole SGR sequence", "[render][escape]")
{
    REQUIRE(render::escape_length(csi("0m")) == 4);
    REQUIRE(render::escape_length(csi("38;2;1;2;3m")) == 13);
}

TEST_CASE("the scanner ends on any final byte, not only m", "[render][escape]")
{
    // The Go tool terminated on `m` alone and ate the rest of the line on
    // everything else. These are that regression.
    REQUIRE(render::strip_escapes(csi("2J") + "art") == "art");
    REQUIRE(render::strip_escapes(csi("?25l") + "art") == "art");
    REQUIRE(render::strip_escapes(csi("1;1H") + "art") == "art");
}

TEST_CASE("the scanner accepts an intermediate byte", "[render][escape]")
{
    REQUIRE(render::strip_escapes(csi("0 q") + "art") == "art");
}

TEST_CASE("a truncated sequence does not read past the end", "[render][escape]")
{
    REQUIRE(render::escape_length(csi("")) == 2);
    REQUIRE(render::escape_length(csi("38;2;")) == 7);
    REQUIRE(render::strip_escapes(csi("38;2;")).empty());
}

TEST_CASE("a lone ESC is one byte", "[render][escape]")
{
    std::string const bare{cli::sgr_escape};

    REQUIRE(render::escape_length(bare) == 1);
    REQUIRE(render::strip_escapes(bare + "art") == "art");
}

TEST_CASE("visible width does not count the escapes", "[render][escape]")
{
    REQUIRE(render::visible_width(csi("38;2;1;2;3m") + "ab" + csi("0m")) == 2);
    REQUIRE(render::visible_width("▀▀▀") == 3);
}

// ---------------------------------------------------------------- decoding

TEST_CASE("a fixture PNG decodes to its dimensions", "[render][image]")
{
    auto const decoded = render::decode(fixtures() / "images" / "red-over-white-2x2.png");

    REQUIRE(decoded.has_value());
    REQUIRE(decoded->width == 2);
    REQUIRE(decoded->height == 2);
    REQUIRE(decoded->pixels.size() == 2 * 2 * render::channels);
}

TEST_CASE("a file that is not an image is an error, not a crash", "[render][image]")
{
    auto const decoded = render::decode(fixtures() / "not-a-deck" / "placeholder.txt");

    REQUIRE_FALSE(decoded.has_value());
    REQUIRE(decoded.error().contains("cannot decode"));
}

// ---------------------------------------------------------------- cells

TEST_CASE("a solid 2x2 becomes one cell of that color", "[render][quantize]")
{
    constexpr cli::rgb teal{0, 128, 128};

    auto const cells = render::to_cells(solid(2, 2, teal), 1, 1);

    REQUIRE(cells.size() == 1);
    REQUIRE(cells.front().size() == 1);
    REQUIRE(cells.front().front() == render::cell{.top = teal, .bottom = teal});
}

TEST_CASE("the top half is the foreground and the bottom the background", "[render][quantize]")
{
    auto const decoded = render::decode(fixtures() / "images" / "red-over-white-2x2.png");
    REQUIRE(decoded.has_value());

    auto const cells = render::to_cells(*decoded, 1, 1);

    REQUIRE(cells.size() == 1);
    REQUIRE(cells.front().front().top == cli::rgb{255, 0, 0});
    REQUIRE(cells.front().front().bottom == cli::rgb{255, 255, 255});
}

TEST_CASE("cells are only produced for a grid the raster actually fits", "[render][quantize]")
{
    REQUIRE(render::to_cells(solid(2, 2, {}), 2, 2).empty());
    REQUIRE(render::to_cells(solid(2, 2, {}), 0, 0).empty());
}

TEST_CASE("quantize resamples onto the grid it was asked for", "[render][quantize]")
{
    auto const cells = render::quantize(solid(64, 64, cli::rgb{200, 100, 50}), 4, 3);

    REQUIRE(cells.has_value());
    REQUIRE(cells->size() == 3);
    REQUIRE(cells->front().size() == 4);
}

// ---------------------------------------------------------------- painting

TEST_CASE("a run of identical cells emits one escape, not one per cell", "[render][paint]")
{
    render::grid const row{
        std::vector<render::cell>(5, render::cell{.top = {1, 2, 3}, .bottom = {4, 5, 6}})
    };

    auto const lines = render::paint(row, cli::color_depth::truecolor);

    REQUIRE(lines.size() == 1);

    // One escape to set the pair, one to reset it.
    REQUIRE(render::strip_escapes(lines.front()) == "▀▀▀▀▀");
    REQUIRE(std::ranges::count(lines.front(), cli::sgr_escape) == 2);
}

TEST_CASE("every painted line ends in a single reset", "[render][paint]")
{
    render::grid const rows{
        {render::cell{.top = {1, 2, 3}, .bottom = {4, 5, 6}}},
        {render::cell{.top = {7, 8, 9}, .bottom = {4, 5, 6}}},
    };

    for (auto const& line : render::paint(rows, cli::color_depth::indexed_256))
        REQUIRE(line.ends_with(csi("0m")));
}

TEST_CASE("no color depth paints no escapes at all", "[render][paint]")
{
    render::grid const row{{render::cell{.top = {1, 2, 3}, .bottom = {4, 5, 6}}}};

    auto const lines = render::paint(row, cli::color_depth::none);

    REQUIRE(lines.front() == "▀");
}

// ---------------------------------------------------------------- color sinks

TEST_CASE("the 256 sink uses the cube and the grey ramp", "[render][color]")
{
    REQUIRE(cli::index_256({0, 0, 0}) == 16);
    REQUIRE(cli::index_256({255, 255, 255}) == 231);
    REQUIRE(cli::index_256({255, 0, 0}) == 196);

    // A mid grey is nearer a ramp step than any cube level.
    REQUIRE(cli::index_256({128, 128, 128}) >= 232);
    REQUIRE(cli::index_256({128, 128, 128}) <= 255);
}

TEST_CASE("the 16 sink picks the nearest of the sixteen", "[render][color]")
{
    REQUIRE(cli::index_16({0, 0, 0}) == 0);
    REQUIRE(cli::index_16({255, 255, 255}) == 15);
    REQUIRE(cli::index_16({250, 5, 5}) == 9);
    REQUIRE(cli::index_16({120, 120, 120}) == 8);
}

TEST_CASE("each depth spells its colors in its own vocabulary", "[render][color]")
{
    render::grid const row{{render::cell{.top = {255, 0, 0}, .bottom = {0, 0, 0}}}};

    REQUIRE(render::paint(row, cli::color_depth::truecolor).front().contains("38;2;255;0;0"));
    REQUIRE(render::paint(row, cli::color_depth::indexed_256).front().contains("38;5;196"));
    REQUIRE(render::paint(row, cli::color_depth::ansi16).front().contains(csi("91;40m")));
}

// ---------------------------------------------------------------- geometry

TEST_CASE("columns follow the rows through the aspect ratio", "[render][layout]")
{
    // A half-block row is two pixels tall: 2 * 20 * 0.5789 rounds to 23.
    auto const size = render::art_dimensions(20, arcana::default_aspect_ratio, 0);

    REQUIRE(size.rows == 20);
    REQUIRE(size.columns == 23);
}

TEST_CASE("a different aspect ratio gets different art", "[render][layout]")
{
    REQUIRE(render::art_dimensions(20, 1.0, 0).columns == 40);
    REQUIRE(render::art_dimensions(20, 0.25, 0).columns == 10);

    // A deck that declares nonsense still gets drawn.
    REQUIRE(render::art_dimensions(20, 0.0, 0).columns == 23);
}

TEST_CASE("a narrow terminal shrinks the art, not the panel", "[render][layout]")
{
    auto const size = render::art_dimensions(40, 0.5, 40);

    REQUIRE(size.columns == 40 - render::gutter - render::panel_floor);
    REQUIRE(size.rows < 40);
}

TEST_CASE("a terminal too narrow for a panel gets no art", "[render][layout]")
{
    REQUIRE(render::art_dimensions(20, 0.5, 20).empty());
    REQUIRE(render::art_dimensions(0, 0.5, 0).empty());
}
