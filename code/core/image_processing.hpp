#pragma once

#include "image.hpp"
#include "pixel_formats.hpp"
#include <vector>

namespace PG
{

enum class CompositeType : uint8_t
{
    REMAP,

    COUNT
};

enum class ColorSpace : uint8_t
{
    INFER, // linear for 1 and 2 channel textures, sRGB for 3 and 4 channel textures
    LINEAR,
    SRGB,

    COUNT
};

struct Remap
{
    Channel from;
    Channel to;
};

struct SourceImage
{
    std::string filename;
    ColorSpace colorSpace = ColorSpace::INFER;
    std::vector<Remap> remaps; // only needed when CompositeType == REMAP
};

struct CompositeImageInput
{
    CompositeType compositeType;
    bool flipVertically = false;

    // Must be set to SRGB or LINEAR. If input channel color space do not match, they will be converted to the output space.
    // The exception is the alpha channel. Input and output alpha channels are assumed to be linear, and will not be converted at all
    ColorSpace outputColorSpace = ColorSpace::COUNT;
    std::vector<SourceImage> sourceImages;
};

FloatImage2D CompositeImage( const CompositeImageInput& input );

ImageFormat PixelFormatToImageFormat( PixelFormat pixelFormat );

} // namespace PG
