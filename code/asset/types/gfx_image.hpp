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
    NORMAL_ROUGHNESS,
    ENVIRONMENT_MAP,
    ENVIRONMENT_MAP_IRRADIANCE,
    ENVIRONMENT_MAP_REFLECTION_PROBE,
    UI,

    NUM_IMAGE_SEMANTICS
};

bool IsSemanticComposite( GfxImageSemantic semantic );

enum class GfxImageFilterMode : uint8_t
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
    float compositeScales[4]           = { 1.0f, 1.0f, 1.0f, 1.0f };
    Channel compositeSourceChannels[4] = { Channel::COUNT, Channel::COUNT, Channel::COUNT, Channel::COUNT };
};

struct GfxImage : public BaseAsset
{
    bool Load( const BaseAssetCreateInfo* baseInfo ) override;
    bool FastfileLoad( Serializer* serializer ) override;
    bool FastfileSave( Serializer* serializer ) const override;
    void Free() override;
    unsigned char* GetPixels( uint32_t face, uint32_t mip, uint32_t depthLevel = 0 ) const;
    void UploadToGpu();

    size_t totalSizeInBytes = 0;
    unsigned char* pixels   = nullptr; // stored mip0face0, mip0face1, mip0face2... mip1face0, mip1face1, etc
    uint32_t width          = 0;
    uint32_t height         = 0;
    uint32_t depth          = 0;
    uint32_t mipLevels      = 0;
    uint32_t numFaces       = 0;
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
size_t CalculateTotalFaceSizeWithMips( uint32_t width, uint32_t height, PixelFormat format, uint32_t numMips = 0 );
size_t CalculateTotalImageBytes(
    PixelFormat format, uint32_t width, uint32_t height, uint32_t depth = 1, uint32_t arrayLayers = 1, uint32_t mipLevels = 1 );
size_t CalculateTotalImageBytes( const GfxImage& img );
PixelFormat ImageFormatToPixelFormat( ImageFormat imgFormat, bool isSRGB );

GfxImage RawImage2DMipsToGfxImage( const std::vector<RawImage2D>& mips, PixelFormat format );
GfxImage DecompressGfxImage( const GfxImage& image );

} // namespace PG
