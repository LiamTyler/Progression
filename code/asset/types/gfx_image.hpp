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

enum class ImageInputType
{
    REGULAR_2D,
    EQUIRECTANGULAR,
    FLATTENED_CUBEMAP,
    INDIVIDUAL_FACES,

    NUM_IMAGE_INPUT_TYPES
};

struct GfxImage : public BaseAsset
{
    bool Load( const BaseAssetCreateInfo* baseInfo ) override;
    bool FastfileLoad( Serializer* serializer ) override;
    bool FastfileSave( Serializer* serializer ) const override;
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

struct GfxImageCreateInfo : public BaseAssetCreateInfo
{
    ImageInputType inputType = ImageInputType::REGULAR_2D;
    std::string filename;
    std::string faceFilenames[6];
    GfxImageSemantic semantic  = GfxImageSemantic::DIFFUSE;
    Gfx::ImageType imageType   = Gfx::ImageType::TYPE_2D;
    PixelFormat dstPixelFormat = PixelFormat::INVALID; // Use src format if this == INVALID
    bool flipVertically        = true;
};


uint32_t CalculateNumMips( uint32_t width, uint32_t height );
// if numMuips is unspecified (0), assume all mips are in use
size_t CalculateTotalFaceSizeWithMips( uint32_t width, uint32_t height, PixelFormat format, uint32_t numMips = 0 );
size_t CalculateTotalImageBytes( PixelFormat format, uint32_t width, uint32_t height, uint32_t depth = 1, uint32_t arrayLayers = 1, uint32_t mipLevels = 1 );

} // namespace PG
