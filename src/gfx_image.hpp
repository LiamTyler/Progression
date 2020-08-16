#pragma once

#include "pixel_formats.hpp"
#include <string>

#define GFX_INVALID_TEXTURE_HANDLE (~0u)
typedef uint32_t GfxTextureHandle;

namespace Progression
{


enum class GfxImageSemantic
{
    DIFFUSE,
    NORMAL
};


enum class GfxImageType : uint8_t
{
    TYPE_1D            = 0,
    TYPE_1D_ARRAY      = 1,
    TYPE_2D            = 2,
    TYPE_2D_ARRAY      = 3,
    TYPE_CUBEMAP       = 4,
    TYPE_CUBEMAP_ARRAY = 5,
    TYPE_3D            = 6,

    NUM_IMAGE_TYPES
};


struct GfxImage
{
    std::string name;
    int width     = 0;
    int height    = 0;
    int depth     = 0;
    int mipLevels = 0;
    int numFaces  = 0;
    unsigned char* pixels = nullptr;
    PixelFormat pixelFormat;
    GfxImageType imageType;
    GfxTextureHandle textureHandle = GFX_INVALID_TEXTURE_HANDLE;
};


struct GfxImageCreateInfo
{
    std::string name;
    std::string filename;
    GfxImageSemantic semantic;
    GfxImageType imageType;
    PixelFormat dstPixelFormat = PixelFormat::INVALID; // Use src format if this == INVALID
};

bool Load_GfxImage( GfxImage* image, const GfxImageCreateInfo& createInfo );


} // namespace Progression
