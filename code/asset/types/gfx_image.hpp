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

enum class GfxImageSemantic : u8
{
    COLOR,
    GRAY,
    ALBEDO_METALNESS,
    NORMAL_ROUGHNESS,
    ENVIRONMENT_MAP,
    ENVIRONMENT_MAP_IRRADIANCE,
    ENVIRONMENT_MAP_REFLECTION_PROBE,
    UI,

    NUM_IMAGE_SEMANTICS
};

bool IsSemanticComposite( GfxImageSemantic semantic );

enum class GfxImageFilterMode : u8
{
    NEAREST,
    BILINEAR,
    TRILINEAR,

    COUNT
};

struct GfxImageCreateInfo : public BaseAssetCreateInfo
{
    std::string filenames[6];
    GfxImageSemantic semantic     = GfxImageSemantic::COLOR;
    PixelFormat dstPixelFormat    = PixelFormat::INVALID; // Format automatically chosen if dstPixelFormat == INVALID
    bool flipVertically           = true;
    bool clampHorizontal          = false;
    bool clampVertical            = false;
    GfxImageFilterMode filterMode = GfxImageFilterMode::TRILINEAR;

    // for composite maps, like ALBEDO_METALNESS
    f32 compositeScales[4]             = { 1.0f, 1.0f, 1.0f, 1.0f };
    Channel compositeSourceChannels[4] = { Channel::COUNT, Channel::COUNT, Channel::COUNT, Channel::COUNT };
};

struct GfxImage : public BaseAsset
{
    bool Load( const BaseAssetCreateInfo* baseInfo ) override;
    bool FastfileLoad( Serializer* serializer ) override;
    bool FastfileSave( Serializer* serializer ) const override;
    void Free() override;
    u8* GetPixels( u32 face, u32 mip, u32 depthLevel = 0 ) const;
    void UploadToGpu();

    size_t totalSizeInBytes = 0;
    u8* pixels              = nullptr; // stored mip0face0, mip0face1, mip0face2... mip1face0, mip1face1, etc
    u32 width               = 0;
    u32 height              = 0;
    u32 depth               = 0;
    u32 mipLevels           = 0;
    u32 numFaces            = 0;
    PixelFormat pixelFormat;
    ImageType imageType;
    bool clampHorizontal;
    bool clampVertical;
    GfxImageFilterMode filterMode;

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

// if numMips is unspecified (0), assume all mips are in use
size_t CalculateTotalFaceSizeWithMips( u32 width, u32 height, PixelFormat format, u32 numMips = 0 );
size_t CalculateTotalImageBytes( PixelFormat format, u32 width, u32 height, u32 depth = 1, u32 arrayLayers = 1, u32 mipLevels = 1 );
size_t CalculateTotalImageBytes( const GfxImage& img );
PixelFormat ImageFormatToPixelFormat( ImageFormat imgFormat, bool isSRGB );

void RawImage2DMipsToGfxImage( GfxImage& image, const std::vector<RawImage2D>& mips, PixelFormat format );
void DecompressGfxImage( const GfxImage& compressedImage, GfxImage& decompressedImage );

} // namespace PG
