// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// Decoding and resampling raster card art.

#pragma once

#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>
#include <vector>

namespace cartomancer::render
{

// Bytes per pixel in a decoded raster.
inline constexpr int channels = 4;

// A decoded raster, RGBA and row-major, with unassociated alpha.
struct raster
{
    int width = 0;
    int height = 0;

    // width * height * channels bytes.
    std::vector<std::uint8_t> pixels;

    [[nodiscard]] bool empty() const noexcept
    {
        return width <= 0 || height <= 0;
    }
};

// Decode one image file.
//
// SVG is never passed here: the search path is best_raster_for_height() only,
// so scalable/ artwork is not a candidate in the first place.
[[nodiscard]] std::expected<raster, std::string> decode(std::filesystem::path const& file);

// Resample to exactly width by height.
[[nodiscard]] std::expected<raster, std::string> resample(
    raster const& source, int width, int height
);

}  // namespace cartomancer::render
