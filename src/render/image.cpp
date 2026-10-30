// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "image.hpp"

// The one translation unit that carries stb's implementations.
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_ONLY_BMP
#define STBI_ONLY_GIF
#define STB_IMAGE_RESIZE_IMPLEMENTATION

#include <stb_image.h>
#include <stb_image_resize.h>

#include <expected>
#include <format>
#include <string>
#include <utility>

namespace cartomancer::render
{

namespace
{

// The largest raster we will decode, so that a hostile or simply enormous
// image cannot be turned into gigabytes of pixels on the way to a 40-column
// approximation of it.
constexpr int size_limit = 20000;

}  // namespace

std::expected<raster, std::string> decode(std::filesystem::path const& file)
{
    int width = 0;
    int height = 0;
    int found = 0;

    auto const name = file.string();
    unsigned char* const pixels = ::stbi_load(name.c_str(), &width, &height, &found, channels);

    if (pixels == nullptr)
    {
        char const* const why = ::stbi_failure_reason();
        return std::unexpected(
            std::format("cannot decode {}: {}", name, why == nullptr ? "unreadable" : why)
        );
    }

    raster decoded{.width = width, .height = height, .pixels = {}};

    auto const count = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) *
                       static_cast<std::size_t>(channels);
    decoded.pixels.assign(pixels, pixels + count);

    ::stbi_image_free(pixels);

    return decoded;
}

std::expected<raster, std::string> resample(raster const& source, int width, int height)
{
    if (source.empty())
        return std::unexpected("cannot resample an empty image");

    if (width <= 0 || height <= 0 || width > size_limit || height > size_limit)
        return std::unexpected(std::format("cannot resample to {}x{}", width, height));

    raster scaled{.width = width, .height = height, .pixels = {}};
    scaled.pixels.resize(
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height) *
        static_cast<std::size_t>(channels)
    );

    // Go used resize.Lanczos3; stb's v1 resizer has no Lanczos, so this is a
    // mitchell/cubic default and not the same filter. The prescale still does
    // the real work of averaging away detail the half-block grid cannot hold,
    // but do not read this as parity with the deleted tool.
    int const worked = ::stbir_resize_uint8_srgb(
        source.pixels.data(), source.width, source.height, 0, scaled.pixels.data(), width, height,
        0, channels, /*alpha_channel=*/channels - 1, /*flags=*/0
    );

    if (worked == 0)
        return std::unexpected("resampling failed");

    return scaled;
}

}  // namespace cartomancer::render
