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
    unsigned char* pixels = nullptr; // stored face0Mip0,face0Mip1,face0Mip2...face1Mip0,face1Mip1,etc
    PixelFormat pixelFormat;
    Gfx::ImageType imageType;

    Gfx::Texture gpuTexture;
};

enum CubemapFaceIndex
{
    CUBEMAP_FACE_BACK   = 0,
    CUBEMAP_FACE_LEFT   = 1,
    CUBEMAP_FACE_FRONT  = 2,
    CUBEMAP_FACE_RIGHT  = 3,
    CUBEMAP_FACE_TOP    = 4,
    CUBEMAP_FACE_BOTTOM = 5,
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

// if numMuips is unspecified (0), assume all mips are in use
size_t CalculateTotalFaceSizeWithMips( uint32_t width, uint32_t height, PixelFormat format, uint32_t numMips = 0 );
size_t CalculateTotalImageBytes( PixelFormat format, uint32_t width, uint32_t height, uint32_t depth = 1, uint32_t arrayLayers = 1, uint32_t mipLevels = 1 );

} // namespace PG
