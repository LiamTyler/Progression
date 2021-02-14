#pragma once

#include "asset/types/base_asset.hpp"
#include "core/pixel_formats.hpp"
#include "renderer/graphics_api/texture.hpp"

class Serializer;

namespace PG
{

enum class GfxImageSemantic
{
    DIFFUSE,
    NORMAL,
    METALNESS,
    ROUGHNESS,

    ENVIRONMENT_MAP,

    NUM_IMAGE_SEMANTICS
};

struct GfxImage : public Asset
{
    void Free() override;
    unsigned char* GetPixels( uint32_t face, uint32_t mip, uint32_t depthLevel = 0 ) const;
    void UploadToGpu();

    uint32_t width     = 0;
    uint32_t height    = 0;
    uint32_t depth     = 0;
    uint32_t mipLevels = 0;
    uint32_t numFaces  = 0;
    size_t totalSizeInBytes = 0;
    unsigned char* pixels = nullptr;
    PixelFormat pixelFormat;
    Gfx::ImageType imageType;

    Gfx::Texture gpuTexture;
};

struct GfxImageCreateInfo
{
    std::string name;
    std::string filename;
    GfxImageSemantic semantic  = GfxImageSemantic::DIFFUSE;
    Gfx::ImageType imageType   = Gfx::ImageType::TYPE_2D;
    PixelFormat dstPixelFormat = PixelFormat::INVALID; // Use src format if this == INVALID
    bool flipVertically        = true;
};

bool GfxImage_Load( GfxImage* image, const GfxImageCreateInfo& createInfo );

bool Fastfile_GfxImage_Load( GfxImage* image, Serializer* serializer );

bool Fastfile_GfxImage_Save( const GfxImage * const image, Serializer* serializer );


} // namespace PG
