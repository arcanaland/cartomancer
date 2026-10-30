// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// How big the art gets to be.

#pragma once

#include <cstddef>

namespace cartomancer::render
{

// Columns between the art and the info panel.
inline constexpr std::size_t gutter = 2;

// The narrowest info panel worth printing beside art. Below this the art is
// dropped rather than the panel: the panel is the part that always says
// something.
inline constexpr std::size_t panel_floor = 24;

// The art's size in terminal cells.
struct art_size
{
    std::size_t columns = 0;
    std::size_t rows = 0;

    [[nodiscard]] bool empty() const noexcept
    {
        return columns == 0 || rows == 0;
    }
};

// The cell dimensions of a card's art.
//
// rows is what --lines or the terminal asked for. Columns follow from the
// deck's aspect ratio, remembering that a half-block row is two pixels tall
// and a column one pixel wide, so columns = 2 * rows * ratio. Both shrink
// together until the art, the gutter and a readable panel fit in width. The
// Go tool hardcoded 40x32 and ignored the deck entirely.
//
// @param width 0 for an unbounded output, as for a pipe
[[nodiscard]] art_size art_dimensions(
    std::size_t rows, double aspect_ratio, std::size_t width
) noexcept;

}  // namespace cartomancer::render
