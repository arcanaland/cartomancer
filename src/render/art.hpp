// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// Choosing and drawing one card's artwork.

#pragma once

#include "cli/color.hpp"
#include "layout.hpp"

#include <arcana/card.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace cartomancer::render
{

// A card's art, ready to print.
struct artwork
{
    // Empty when the card has no art we can draw.
    std::vector<std::string> lines;

    // The visible width of the widest line, escapes not counted.
    std::size_t columns = 0;

    // The image root we drew from, e.g. "ansi32" or "h1200".
    std::string source;

    // Set when the deck did ship artwork and it could not be used. Distinct
    // from having none at all, and worth telling the user about.
    std::optional<std::string> problem;
};

// The card's art at the requested size.
//
// ANSI art the deck ships wins when it has any: hand-authored art beats our
// approximation of a photograph. A raster is quantized only when there is no
// ANSI file. Scalable artwork is never a candidate — the Go tool resolved
// scalable/ first and then failed to decode it.
[[nodiscard]] artwork draw_card(arcana::card const& subject, art_size size, cli::color_depth depth);

}  // namespace cartomancer::render
