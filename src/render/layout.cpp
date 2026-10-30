// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "layout.hpp"

#include <arcana/deck.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace cartomancer::render
{

namespace
{

// A half-block row is two pixels tall against a column's one.
constexpr double subpixel_rows = 2.0;

[[nodiscard]] std::size_t rounded(double value) noexcept
{
    return value <= 0.0 ? 0 : static_cast<std::size_t>(std::llround(value));
}

}  // namespace

art_size art_dimensions(std::size_t rows, double aspect_ratio, std::size_t width) noexcept
{
    if (rows == 0)
        return {};

    // A deck that declares a nonsensical ratio still gets art.
    double const ratio = aspect_ratio > 0.0 ? aspect_ratio : arcana::default_aspect_ratio;

    auto columns =
        std::max<std::size_t>(1, rounded(subpixel_rows * static_cast<double>(rows) * ratio));

    if (width == 0)
        return {.columns = columns, .rows = rows};

    auto const overhead = gutter + panel_floor;
    if (width <= overhead)
        return {};

    if (auto const budget = width - overhead; columns > budget)
    {
        columns = budget;
        rows = std::max<std::size_t>(
            1, rounded(static_cast<double>(columns) / (subpixel_rows * ratio))
        );
    }

    return {.columns = columns, .rows = rows};
}

}  // namespace cartomancer::render
