// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "art.hpp"

#include "escape.hpp"
#include "image.hpp"
#include "quantize.hpp"

#include <algorithm>
#include <format>
#include <fstream>
#include <ios>
#include <iterator>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace cartomancer::render
{

namespace
{

// The widest line, escapes not counted.
[[nodiscard]] std::size_t widest(std::vector<std::string> const& lines)
{
    std::size_t columns = 0;
    for (auto const& one : lines) columns = std::max(columns, visible_width(one));
    return columns;
}

// The deck's own ANSI art, read verbatim.
[[nodiscard]] std::optional<artwork> from_ansi(arcana::card_image const& source)
{
    std::ifstream file(source.path, std::ios::binary);
    if (!file)
        return std::nullopt;

    std::vector<std::string> lines;
    std::string one;
    while (std::getline(file, one))
    {
        if (one.ends_with('\r'))
            one.pop_back();

        lines.push_back(one);
    }

    if (lines.empty())
        return std::nullopt;

    return artwork{
        .lines = std::move(lines),
        .columns = 0,
        .source = source.source_dir,
        .problem = std::nullopt,
    };
}

// A raster, resampled and averaged down to half-block cells.
[[nodiscard]] artwork from_raster(
    arcana::card_image const& source, art_size size, cli::color_depth depth
)
{
    artwork drawn{.lines = {}, .columns = 0, .source = source.source_dir, .problem = std::nullopt};

    auto decoded = decode(source.path);
    if (!decoded.has_value())
    {
        drawn.problem = std::move(decoded).error();
        return drawn;
    }

    auto cells = quantize(*decoded, static_cast<int>(size.columns), static_cast<int>(size.rows));
    if (!cells.has_value())
    {
        drawn.problem = std::move(cells).error();
        return drawn;
    }

    drawn.lines = paint(*cells, depth);
    drawn.columns = size.columns;

    return drawn;
}

}  // namespace

artwork draw_card(arcana::card const& subject, art_size size, cli::color_depth depth)
{
    if (size.empty() || depth == cli::color_depth::none)
        return {};

    if (auto const ansi = subject.best_ansi_for_lines(static_cast<int>(size.rows)))
        if (auto drawn = from_ansi(*ansi))
        {
            drawn->columns = widest(drawn->lines);
            return std::move(*drawn);
        }

    // The raster search wants a pixel height, and a half-block row is two
    // pixels of one.
    auto const target = static_cast<int>(size.rows) * 2;

    if (auto const image = subject.best_raster_for_height(target))
        return from_raster(*image, size, depth);

    return {};
}

}  // namespace cartomancer::render
