#include "asset/types/gfx_image.hpp"
#include "asset/image.hpp"
#include "core/assert.hpp"
#include "core/image_calculate_size.hpp"
#include "renderer/r_globals.hpp"
#include "utils/cpu_profile.hpp"
#include "utils/logger.hpp"
#include "utils/serializer.hpp"

#include "glm/glm.hpp"
#include "stb/stb_image_write.h"
#include "stb/stb_image_resize.h"

#include <algorithm>

typedef unsigned int uint;

union FP32
{
    uint u;
    float f;
    struct
    {
        uint Mantissa : 23;
        uint Exponent : 8;
        uint Sign : 1;
    };
};

union FP16
{
    unsigned short u;
    struct
    {
        uint Mantissa : 10;
        uint Exponent : 5;
        uint Sign : 1;
    };
};

// Original ISPC reference version; this always rounds ties up.
static FP16 float_to_half_full( float f32 )
{
    FP32 f;
    f.f = f32;
    FP16 o = { 0 };

    // Based on ISPC reference code (with minor modifications)
    if ( f.Exponent == 0 ) // Signed zero/denormal (which will underflow)
    {
        o.Exponent = 0;
    }
    else if ( f.Exponent == 255 ) // Inf or NaN (all exponent bits set)
    {
        o.Exponent = 31;
        o.Mantissa = f.Mantissa ? 0x200 : 0; // NaN->qNaN and Inf->Inf
    }
    else // Normalized number
    {
        // Exponent unbias the single, then bias the halfp
        int newexp = f.Exponent - 127 + 15;
        if ( newexp >= 31 ) // Overflow, return signed infinity
        {
            o.Exponent = 31;
        }
        else if ( newexp <= 0 ) // Underflow
        {
            if ( (14 - newexp) <= 24 ) // Mantissa might be non-zero
            {
                uint mant = f.Mantissa | 0x800000; // Hidden 1 bit
                o.Mantissa = mant >> (14 - newexp);
                if ( (mant >> (13 - newexp)) & 1 ) // Check for rounding
                {
                    o.u++; // Round, might overflow into exp bit, but this is OK
                }
            }
        }
        else
        {
            o.Exponent = newexp;
            o.Mantissa = f.Mantissa >> 13;
            if ( f.Mantissa & 0x1000 ) // Check for rounding
            {
                o.u++; // Round, might overflow to inf, this is OK
            }
        }
    }

    o.Sign = f.Sign;
    return o;
}


namespace PG
{

void GfxImage::Free()
{
    if ( pixels )
    {
        free( pixels );
        pixels = nullptr;
    }
#if !USING( COMPILING_CONVERTER )
    gpuTexture.Free();
#endif // #if !USING( COMPILING_CONVERTER )
}


unsigned char* GfxImage::GetPixels( uint32_t face, uint32_t mip, uint32_t depthLevel ) const
{
    PG_ASSERT( face == 0, "Multiple faces not supported yet" );
    PG_ASSERT( depthLevel == 0, "Texture arrays not supported yet" );
    PG_ASSERT( !PixelFormatIsCompressed( pixelFormat ), "Compression not supported yet" );
    PG_ASSERT( mip < mipLevels );
    PG_ASSERT( pixels );

    int w = width;
    int h = height;
    int bytesPerPixel = NumBytesPerPixel( pixelFormat );
    size_t offset = 0;
    for ( uint32_t mipLevel = 0; mipLevel < mip; ++mipLevel )
    {
        offset += w * h * bytesPerPixel;
        w = std::max( 1, w >> 1 );
        h = std::max( 1, h >> 1 );
    }

    return pixels + offset;
}


void GfxImage::UploadToGpu()
{
#if USING( COMPILING_CONVERTER )
    PG_ASSERT( false, "Render system not available in converter" );
#else // #if USING( COMPILING_CONVERTER )
    using namespace Gfx;
    PG_ASSERT( pixels );
    
    if ( gpuTexture )
    {
        gpuTexture.Free();
    }

    TextureDescriptor desc;
    desc.format      = pixelFormat;
    desc.type        = imageType;
    desc.width       = width;
    desc.height      = height;
    desc.depth       = depth;
    desc.arrayLayers = numFaces;
    desc.mipLevels   = mipLevels;
    desc.usage       = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    desc.addToBindlessArray = imageType == ImageType::TYPE_2D;

    gpuTexture = r_globals.device.NewTextureFromBuffer( desc, pixels, name );
    PG_ASSERT( gpuTexture );
    free( pixels );
    pixels = nullptr;

#endif // #else // #if USING( COMPILING_CONVERTER )
}


static PixelFormat ChoosePixelFormatFromSemantic( const GfxImageSemantic& semantic )
{
    PixelFormat format = PixelFormat::INVALID;
    switch ( semantic )
    {
    case GfxImageSemantic::DIFFUSE:
        format = PixelFormat::R8_G8_B8_A8_SRGB;
        break;
    case GfxImageSemantic::NORMAL:
        format = PixelFormat::R8_G8_B8_A8_UNORM;
        break;
    case GfxImageSemantic::METALNESS:
    case GfxImageSemantic::ROUGHNESS:
        format = PixelFormat::R8_UNORM;
        break;
    case GfxImageSemantic::ENVIRONMENT_MAP:
        format = PixelFormat::R16_G16_B16_A16_FLOAT;
        break;
    default:
        LOG_ERR( "Semantic (%d) unknown when deciding final image format", semantic );
        break;
    }

    return format;
}


static void NormalizeImage( glm::vec4* pixels, int width, int height )
{
    for ( int row = 0; row < height; ++row )
    {
        for ( int col = 0; col < width; ++col )
        {
            const glm::vec4& pixel = pixels[row * width + col];
            glm::vec3 normal = pixel;
            normal = normalize( normal );
            pixels[row * width + col] = glm::vec4( normal, pixel.a );
        }
    }
}


static void GenerateMipmaps_Float32( const glm::vec4* srcPixels, int width, int height, glm::vec4* dstPixels, GfxImageSemantic semantic )
{
    PG_ASSERT( srcPixels );
    PG_ASSERT( width != 0 && height != 0 );
    int w = width;
    int h = height;
    int numMips = CalculateNumMips( width, height );
    
    int lastW, lastH;
    size_t lastOffset;
    size_t currentOffset = 0;
    for ( int mipLevel = 0; mipLevel < numMips; ++mipLevel )
    {
        if ( mipLevel == 0 )
        {
            if ( srcPixels != dstPixels )
            {
                memcpy( dstPixels, srcPixels, w * h * sizeof( glm::vec4 ) );
            }
        }
        else
        {
            float* currentDstImg = reinterpret_cast< float* >( dstPixels + currentOffset );
            float* currentSrcImage = reinterpret_cast< float* >( dstPixels + lastOffset );
            int flags = 0;
            int alphaChannel = 3;
            stbir_resize_float_generic( currentSrcImage, lastW, lastH, lastW * sizeof( glm::vec4 ), currentDstImg, w, h, w * sizeof( glm::vec4 ), 4, alphaChannel, flags, STBIR_EDGE_CLAMP, STBIR_FILTER_MITCHELL, STBIR_COLORSPACE_LINEAR, NULL );
        }

        // renormalize normal maps
        if ( semantic == GfxImageSemantic::NORMAL )
        {
            NormalizeImage( dstPixels + currentOffset, w, h );
        }

        lastW = w;
        lastH = h;
        lastOffset = currentOffset;
        currentOffset += w * h;
        w = std::max( 1, w >> 1 );
        h = std::max( 1, h >> 1 );
    }
}


static void ConvertRGBA32Float_SingleMip( unsigned char* outputImage, int width, int height, glm::vec4* inputPixels, PixelFormat format )
{
    PG_ASSERT( inputPixels );
    PG_ASSERT( outputImage );
    PG_ASSERT( format != PixelFormat::INVALID && format != PixelFormat::NUM_PIXEL_FORMATS );
    PG_ASSERT( !PixelFormatHasDepthFormat( format ) );
    PG_ASSERT( !PixelFormatIsCompressed( format ), "Compression not supported yet" );
    
    int numChannels     = NumChannelsInPixelFromat( format );
    int bytesPerChannel = NumBytesPerChannel( format );
    int bytesPerPixel   = NumBytesPerPixel( format );
    bool isNormalized   = PixelFormatIsNormalized( format );
    bool isUnsigned     = PixelFormatIsUnsigned( format );
    bool isSrgb         = PixelFormatIsSrgb( format );
    bool isFloat        = PixelFormatIsFloat( format );
    int channelOrder[4];
    GetRGBAOrder( format, channelOrder );

    for ( int row = 0; row < height; ++row )
    {
        for ( int col = 0; col < width; ++col )
        {
            glm::vec4 pixel = inputPixels[row * width + col];
            for ( int channel = 0; channel < numChannels; ++channel )
            {
                float x = pixel[channelOrder[channel]];
                if ( isNormalized )
                {
                    x = isUnsigned ? std::clamp( x, 0.0f, 1.0f ) : std::clamp( x, -1.0f, 1.0f );
                }
                else if ( !isFloat )
                {
                    x = std::roundf( x );
                }
                if ( isSrgb )
                {
                    x = LinearToGammaSRGB( x );
                }

                // s8 = 127, u8 = 255, s16 = 32767, u16 = 65535
                if ( !isFloat && bytesPerChannel != 4 )
                {
                    int scale = (1 << (8*bytesPerChannel - !isUnsigned)) - 1;
                    x *= scale;
                }

                // pack
                int outputByteIndex = bytesPerPixel * (row * width + col) + channelOrder[channel]*bytesPerChannel;
                if ( isFloat )
                {
                    if ( bytesPerChannel == 2 )
                    {
                        FP16 f16 = float_to_half_full( x );
                        *reinterpret_cast< uint16_t* >( outputImage + outputByteIndex ) = f16.u;
                    }
                    else
                    {
                        *reinterpret_cast< float* >( outputImage + outputByteIndex ) = x;
                    }
                }
                else
                {
                    int64_t value = static_cast< int64_t >( x );
                    for ( int byte = 0; byte < bytesPerChannel; ++byte )
                    {
                        outputImage[outputByteIndex + byte] = reinterpret_cast<unsigned char*>( &value )[byte];
                    }
                }
            }
        }
    }
    
}


static void ConvertRGBA32Float_AllMips( unsigned char* outputImage, int width, int height, int numFaces, int numMips, glm::vec4* inputPixels, PixelFormat format )
{
    PG_ASSERT( outputImage && inputPixels );
    int w = width;
    int h = height;
    int bytesPerDstPixel = NumBytesPerPixel( format );
    
    glm::vec4* currentSrcImage     = inputPixels;
    unsigned char* currentDstImage = outputImage;
    for ( int mipLevel = 0; mipLevel < numMips; ++mipLevel )
    {
        for ( int face = 0; face < numFaces; ++face )
        {
            ConvertRGBA32Float_SingleMip( currentDstImage, w, h, currentSrcImage, format );
            currentSrcImage += w * h;
            currentDstImage += w * h * bytesPerDstPixel;
        }
        w = std::max( 1, w >> 1 );
        h = std::max( 1, h >> 1 );
    }
}


static bool Load_GfxImage_2D( GfxImage* gfxImage, const GfxImageCreateInfo& createInfo )
{
    Image2DCreateInfo srcImgCreateInfo;
    srcImgCreateInfo.filename = createInfo.filename;
    srcImgCreateInfo.flipVertically = createInfo.flipVertically;
    Image2D srcImg;
    if ( !srcImg.Load( &srcImgCreateInfo ) )
    {
        return false;
    }

    int w = srcImg.width;
    int h = srcImg.height;
    size_t totalSrcImageSize = CalculateTotalFaceSizeWithMips( w, h, PixelFormat::R32_G32_B32_A32_FLOAT );
    glm::vec4* pixelsAllMips = static_cast< glm::vec4* >( malloc( totalSrcImageSize ) );
    memset( pixelsAllMips, 0, totalSrcImageSize );
    GenerateMipmaps_Float32( srcImg.pixels, w, h, pixelsAllMips, createInfo.semantic );

    gfxImage->width     = w;
    gfxImage->height    = h;
    gfxImage->numFaces  = 1;
    gfxImage->mipLevels = CalculateNumMips( w, h );
    gfxImage->totalSizeInBytes = CalculateTotalFaceSizeWithMips( w, h, gfxImage->pixelFormat );
    gfxImage->pixels = static_cast< unsigned char* >( malloc( gfxImage->totalSizeInBytes ) );
    ConvertRGBA32Float_AllMips( gfxImage->pixels, w, h, gfxImage->numFaces, gfxImage->mipLevels, pixelsAllMips, gfxImage->pixelFormat );
    
    free( pixelsAllMips );

    return true;
}


static bool Load_GfxImage_Cubemap( GfxImage* gfxImage, const GfxImageCreateInfo& createInfo )
{
    ImageCubemapCreateInfo srcImgCreateInfo;
    if ( createInfo.inputType == ImageInputType::EQUIRECTANGULAR )
    {
        srcImgCreateInfo.equirectangularFilename = createInfo.filename;
    }
    else if ( createInfo.inputType == ImageInputType::FLATTENED_CUBEMAP )
    {
        srcImgCreateInfo.flattenedCubemapFilename = createInfo.filename;
    }
    else if ( createInfo.inputType == ImageInputType::INDIVIDUAL_FACES )
    {
        for ( int i = 0; i < 6; ++i )
        {
            srcImgCreateInfo.faceFilenames[i] = createInfo.faceFilenames[i];
        }
    }
    srcImgCreateInfo.flipVertically = createInfo.flipVertically;
    ImageCubemap srcImg;
    if ( !srcImg.Load( &srcImgCreateInfo ) )
    {
        return false;
    }

    int w = srcImg.faces[0].width;
    int h = srcImg.faces[0].height;
    size_t mipChainSizeInBytes = CalculateTotalFaceSizeWithMips( w, h, PixelFormat::R32_G32_B32_A32_FLOAT );
    glm::vec4* pixelsAllMips = static_cast< glm::vec4* >( malloc( 6 * mipChainSizeInBytes ) );
    // Vulkan face order: front, back, up, down, right and lastly left
    int faceOrder[] = { FACE_FRONT, FACE_BACK, FACE_TOP, FACE_BOTTOM, FACE_RIGHT, FACE_LEFT };
    for ( int face = 0; face < 6; ++face )
    {
        glm::vec4* faceMip0 = pixelsAllMips + face * (mipChainSizeInBytes / sizeof( glm::vec4 ));
        memcpy( faceMip0, srcImg.faces[faceOrder[face]].pixels, w * h * sizeof( glm::vec4 ) );
        GenerateMipmaps_Float32( faceMip0, w, h, faceMip0, createInfo.semantic );
    }

    gfxImage->width     = w;
    gfxImage->height    = h;
    gfxImage->mipLevels = CalculateNumMips( w, h );
    gfxImage->numFaces  = 6;
    gfxImage->totalSizeInBytes = 6 * CalculateTotalFaceSizeWithMips( w, h, gfxImage->pixelFormat );
    gfxImage->pixels = static_cast< unsigned char* >( malloc( gfxImage->totalSizeInBytes ) );
    ConvertRGBA32Float_AllMips( gfxImage->pixels, w, h, gfxImage->numFaces, gfxImage->mipLevels, pixelsAllMips, gfxImage->pixelFormat );
    
    free( pixelsAllMips );


    return true;
}


bool GfxImage::Load( const BaseAssetCreateInfo* baseInfo )
{
    PG_ASSERT( baseInfo );
    const GfxImageCreateInfo* createInfo = (const GfxImageCreateInfo*)baseInfo;
    name        = createInfo->name;
    imageType   = createInfo->imageType;
    pixelFormat = createInfo->dstPixelFormat;
    depth       = 1;
    mipLevels   = 1;
    numFaces    = 1;

    if ( createInfo->dstPixelFormat == PixelFormat::INVALID )
    {
        pixelFormat = ChoosePixelFormatFromSemantic( createInfo->semantic );
    }

    bool success;
    if ( createInfo->imageType == Gfx::ImageType::TYPE_2D )
    {
        success = Load_GfxImage_2D( this, *createInfo );
    }
    else if ( createInfo->imageType == Gfx::ImageType::TYPE_CUBEMAP )
    {
        success = Load_GfxImage_Cubemap( this, *createInfo );
    }
    else
    {
        LOG_ERR( "GfxImageType '%d' not supported yet", static_cast< int >( createInfo->imageType ) );
        success = false;
    }

    return success;
}


bool GfxImage::FastfileLoad( Serializer* serializer )
{
    static_assert( sizeof( GfxImage ) == sizeof( std::string ) + 56 + sizeof( Gfx::Texture ), "Don't forget to update this function if added/removed members from GfxImage!" );
    
    PG_ASSERT( serializer );
    serializer->Read( name );
    serializer->Read( width );
    serializer->Read( height );
    serializer->Read( depth );
    serializer->Read( mipLevels );
    serializer->Read( numFaces );
    serializer->Read( totalSizeInBytes );
    serializer->Read( pixelFormat );
    serializer->Read( imageType );
    pixels = static_cast< unsigned char* >( malloc( totalSizeInBytes ) );
    serializer->Read( pixels, totalSizeInBytes );

    UploadToGpu();

    return true;
}


bool GfxImage::FastfileSave( Serializer* serializer ) const
{
    static_assert( sizeof( GfxImage ) == sizeof( std::string ) + 56 + sizeof( Gfx::Texture ), "Don't forget to update this function if added/removed members from GfxImage!" );
    
    PG_ASSERT( serializer );
    PG_ASSERT( pixels );
    serializer->Write( name );
    serializer->Write( width );
    serializer->Write( height );
    serializer->Write( depth );
    serializer->Write( mipLevels );
    serializer->Write( numFaces );
    serializer->Write( totalSizeInBytes );
    serializer->Write( pixelFormat );
    serializer->Write( imageType );
    serializer->Write( pixels, totalSizeInBytes );

    return true;
}


} // namespace PG
