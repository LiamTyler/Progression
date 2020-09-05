#pragma once

#include "assetTypes/base_asset.hpp"
#include "pixel_formats.hpp"

#define GFX_INVALID_TEXTURE_HANDLE (~0u)
typedef uint32_t GfxTextureHandle;

class Serializer;

namespace Progression
{

enum class GfxImageSemantic
{
    DIFFUSE,
    NORMAL,

    NUM_IMAGE_SEMANTICS
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

struct GfxImage : public Asset
{
    void Free() override;
    unsigned char* GetPixels( int face, int mip, int depthLevel = 0 ) const;

    int width     = 0;
    int height    = 0;
    int depth     = 0;
    int mipLevels = 0;
    int numFaces  = 0;
    size_t totalSizeInBytes = 0;
    unsigned char* pixels = nullptr;
    PixelFormat pixelFormat;
    GfxImageType imageType;
    GfxTextureHandle textureHandle = GFX_INVALID_TEXTURE_HANDLE;
};

struct GfxImageCreateInfo
{
    std::string name;
    std::string filename;
    GfxImageSemantic semantic  = GfxImageSemantic::DIFFUSE;
    GfxImageType imageType     = GfxImageType::TYPE_2D;
    PixelFormat dstPixelFormat = PixelFormat::INVALID; // Use src format if this == INVALID
};

bool GfxImage_Load( GfxImage* image, const GfxImageCreateInfo& createInfo );

bool Fastfile_GfxImage_Load( GfxImage* image, Serializer* serializer );

bool Fastfile_GfxImage_Save( const GfxImage * const image, Serializer* serializer );

int CalculateNumMips( int width, int height );

size_t CalculateTotalFaceSizeWithMips( int mip0Width, int mip0Height, PixelFormat format );


} // namespace Progression
