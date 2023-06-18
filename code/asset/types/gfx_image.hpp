#pragma once

#include "asset/types/base_asset.hpp"
#include "core/image_types.hpp"
#include "core/pixel_formats.hpp"
#if USING( GPU_DATA )
#include "renderer/graphics_api/texture.hpp"
#endif // #if USING( GPU_DATA )
#include "ImageLib/image.hpp"

class Serializer;

namespace PG
{

enum class GfxImageSemantic : uint8_t
{
    COLOR,
    GRAY,
    ALBEDO_METALNESS,
    ENVIRONMENT_MAP,
    UI,

    NUM_IMAGE_SEMANTICS
};

bool IsSemanticComposite( GfxImageSemantic semantic );

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
    ImageType imageType;

#if USING( GPU_DATA )
    Gfx::Texture gpuTexture;
#endif // #if USING( GPU_DATA )
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
    std::string filenames[6];
    GfxImageSemantic semantic  = GfxImageSemantic::COLOR;
    PixelFormat dstPixelFormat = PixelFormat::INVALID; // Format automatically chosen if dstPixelFormat == INVALID
    bool flipVertically        = true;
    bool clampHorizontal       = true;
    bool clampVertical         = true;

    // for composite maps, like ALBEDO_METALNESS
    float compositeScales[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    Channel compositeSourceChannels[4] = { Channel::COUNT, Channel::COUNT, Channel::COUNT, Channel::COUNT };
};

// if numMips is unspecified (0), assume all mips are in use
size_t CalculateTotalFaceSizeWithMips( uint32_t width, uint32_t height, PixelFormat format, uint32_t numMips = 0 );
size_t CalculateTotalImageBytes( PixelFormat format, uint32_t width, uint32_t height, uint32_t depth = 1, uint32_t arrayLayers = 1, uint32_t mipLevels = 1 );
PixelFormat ImageFormatToPixelFormat( ImageFormat imgFormat, bool isSRGB );

GfxImage RawImage2DMipsToGfxImage( const std::vector<RawImage2D>& mips, bool isSRGB );
GfxImage DecompressGfxImage( const GfxImage &image );

} // namespace PG
