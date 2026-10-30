// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// Turning a raster into a grid of half-block terminal cells.

#pragma once

#include "cli/color.hpp"
#include "image.hpp"

#include <expected>
#include <string>
#include <string_view>
#include <vector>

namespace cartomancer::render
{

// One terminal cell drawn as an upper half-block: its top half takes the
// foreground color and its bottom half the background.
struct cell
{
    cli::rgb top;
    cli::rgb bottom;

    friend bool operator==(cell const&, cell const&) = default;
};

// U+2580 UPPER HALF BLOCK.
inline constexpr std::string_view half_block = "▀";

// Rows of cells, top to bottom.
using grid = std::vector<std::vector<cell>>;

// Average each 2x2 block of subpixels into one cell.
//
// subpixels must be exactly 2*columns by 2*rows. Alpha is composited over
// black, which is the only background we can name.
[[nodiscard]] grid to_cells(raster const& subpixels, int columns, int rows);

// Resample source onto the cell grid and average it down. Pure: no escapes
// and no I/O, so it can be tested without a terminal.
[[nodiscard]] std::expected<grid, std::string> quantize(
    raster const& source, int columns, int rows
);

// The grid as terminal lines.
//
// Every escape comes from cli::sgr_writer, which emits only what changed and
// ends each line with exactly one reset.
[[nodiscard]] std::vector<std::string> paint(grid const& cells, cli::color_depth depth);

}  // namespace cartomancer::render
