// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "quantize.hpp"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <string>
#include <utility>
#include <vector>

namespace cartomancer::render
{

namespace
{

constexpr int opaque = 255;

// One pixel of source, composited over black.
[[nodiscard]] cli::rgb flatten(raster const& source, int x, int y) noexcept
{
    auto const at = ((static_cast<std::size_t>(y) * static_cast<std::size_t>(source.width)) +
                     static_cast<std::size_t>(x)) *
                    static_cast<std::size_t>(channels);

    int const alpha = source.pixels[at + 3];

    auto const over_black = [alpha](int value)
    { return static_cast<int>((value * alpha) / opaque); };

    return {
        static_cast<std::uint8_t>(over_black(source.pixels[at])),
        static_cast<std::uint8_t>(over_black(source.pixels[at + 1])),
        static_cast<std::uint8_t>(over_black(source.pixels[at + 2])),
    };
}

[[nodiscard]] cli::rgb mean_of(cli::rgb left, cli::rgb right) noexcept
{
    return {
        static_cast<std::uint8_t>((left.red + right.red) / 2),
        static_cast<std::uint8_t>((left.green + right.green) / 2),
        static_cast<std::uint8_t>((left.blue + right.blue) / 2),
    };
}

}  // namespace

grid to_cells(raster const& subpixels, int columns, int rows)
{
    if (columns <= 0 || rows <= 0 || subpixels.width != 2 * columns || subpixels.height != 2 * rows)
        return {};

    grid cells;
    cells.reserve(static_cast<std::size_t>(rows));

    for (int row = 0; row < rows; ++row)
    {
        std::vector<cell> line;
        line.reserve(static_cast<std::size_t>(columns));

        for (int column = 0; column < columns; ++column)
        {
            int const x = 2 * column;
            int const y = 2 * row;

            line.push_back(
                {.top = mean_of(flatten(subpixels, x, y), flatten(subpixels, x + 1, y)),
                 .bottom = mean_of(flatten(subpixels, x, y + 1), flatten(subpixels, x + 1, y + 1))}
            );
        }

        cells.push_back(std::move(line));
    }

    return cells;
}

std::expected<grid, std::string> quantize(raster const& source, int columns, int rows)
{
    if (columns <= 0 || rows <= 0)
        return std::unexpected("cannot render art with no cells");

    auto subpixels = resample(source, 2 * columns, 2 * rows);
    if (!subpixels.has_value())
        return std::unexpected(std::move(subpixels).error());

    return to_cells(*subpixels, columns, rows);
}

std::vector<std::string> paint(grid const& cells, cli::color_depth depth)
{
    std::vector<std::string> lines;
    lines.reserve(cells.size());

    for (auto const& row : cells)
    {
        // One writer per line: a line has to stand on its own, because the
        // gutter beside it and whatever the terminal does at a wrap are not
        // ours to reason about.
        cli::sgr_writer writer(depth);
        std::string line;

        for (auto const& one : row)
        {
            writer.paint(line, one.top, one.bottom);
            line += half_block;
        }

        writer.clear(line);
        lines.push_back(std::move(line));
    }

    return lines;
}

}  // namespace cartomancer::render
