#include "assetTypes/gfx_image.hpp"
#include "assert.hpp"
#include "image.hpp"
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


namespace Progression
{

void GfxImage::Free()
{
    if ( pixels )
    {
        free( pixels );
        pixels = nullptr;
    }
}


unsigned char* GfxImage::GetPixels( int face, int mip, int depthLevel ) const
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
    for ( int mipLevel = 0; mipLevel < mip; ++mipLevel )
    {
        offset += w * h * bytesPerPixel;
        w = std::max( 1, w >> 1 );
        h = std::max( 1, h >> 1 );
    }

    return pixels + offset;
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


static void GenerateMipmaps( const Image& image, glm::vec4* outputPixels, GfxImageSemantic semantic )
{
    //PG_PROFILE_START( GenerateMipmaps );
    PG_ASSERT( image.pixels );
    PG_ASSERT( image.width != 0 && image.height != 0 );
    int w = image.width;
    int h = image.height;
    int numMips = CalculateNumMips( image.width, image.height );
    
    int lastW, lastH;
    size_t lastOffset;
    size_t currentOffset = 0;
    for ( int mipLevel = 0; mipLevel < numMips; ++mipLevel )
    {
        if ( mipLevel == 0 )
        {
            // copy mip 0 into output buffer
            memcpy( outputPixels, image.pixels, w * h * sizeof( glm::vec4 ) );
        }
        else
        {
            float* currentDstImg = reinterpret_cast< float* >( outputPixels + currentOffset );
            float* currentSrcImage = reinterpret_cast< float* >( outputPixels + lastOffset );
            int flags = 0;
            int alphaChannel = 3;
            //stbir_resize_float_generic( currentSrcImage, lastW, lastH, lastW * sizeof( glm::vec4 ), currentDstImg, w, h, w * sizeof( glm::vec4 ), 4, alphaChannel, flags, STBIR_EDGE_CLAMP, STBIR_FILTER_MITCHELL, STBIR_COLORSPACE_LINEAR, NULL );
            stbir_resize_float_generic( (float*)image.pixels, image.width, image.height, image.width * sizeof( glm::vec4 ), currentDstImg, w, h, w * sizeof( glm::vec4 ), 4, alphaChannel, flags, STBIR_EDGE_CLAMP, STBIR_FILTER_MITCHELL, STBIR_COLORSPACE_LINEAR, NULL );
        }

        // renormalize normal maps
        if ( semantic == GfxImageSemantic::NORMAL )
        {
            NormalizeImage( outputPixels + currentOffset, w, h );
        }

        lastW = w;
        lastH = h;
        lastOffset = currentOffset;
        currentOffset += w * h;
        w = std::max( 1, w >> 1 );
        h = std::max( 1, h >> 1 );
    }
    //PG_PROFILE_END( GenerateMipmaps );
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


static void ConvertRGBA32Float_AllMips( unsigned char* outputImage, int width, int height, glm::vec4* inputPixels, PixelFormat format )
{
    //PG_PROFILE_START( ConvertRGBA32Float_AllMips );
    PG_ASSERT( outputImage && inputPixels );
    int w = width;
    int h = height;
    int numMips = CalculateNumMips( width, height );
    int bytesPerDstPixel = NumBytesPerPixel( format );
    
    glm::vec4* currentSrcImage     = inputPixels;
    unsigned char* currentDstImage = outputImage;
    for ( int mipLevel = 0; mipLevel < numMips; ++mipLevel )
    {
        ConvertRGBA32Float_SingleMip( currentDstImage, w, h, currentSrcImage, format );
        currentSrcImage += w * h;
        currentDstImage += w * h * bytesPerDstPixel;
        w = std::max( 1, w >> 1 );
        h = std::max( 1, h >> 1 );
    }
    //PG_PROFILE_END( ConvertRGBA32Float_AllMips );
}


static bool Load_GfxImage_2D( GfxImage* gfxImage, const GfxImageCreateInfo& createInfo )
{
    ImageCreateInfo srcImgCreateInfo;
    srcImgCreateInfo.filename = createInfo.filename;
    Image srcImg;
    if ( !srcImg.Load( &srcImgCreateInfo ) )
    {
        return false;
    }

    int w = srcImg.width;
    int h = srcImg.height;
    size_t totalSrcImageSize = CalculateTotalFaceSizeWithMips( w, h, PixelFormat::R32_G32_B32_A32_FLOAT );
    glm::vec4* srcPixelsAllMips = static_cast< glm::vec4* >( malloc( totalSrcImageSize ) );
    memset( srcPixelsAllMips, 0, totalSrcImageSize );
    GenerateMipmaps( srcImg, srcPixelsAllMips, createInfo.semantic );

    gfxImage->width     = w;
    gfxImage->height    = h;
    gfxImage->mipLevels = CalculateNumMips( w, h );
    gfxImage->totalSizeInBytes = CalculateTotalFaceSizeWithMips( w, h, createInfo.dstPixelFormat );
    gfxImage->pixels = static_cast< unsigned char* >( malloc( gfxImage->totalSizeInBytes ) );
    ConvertRGBA32Float_AllMips( gfxImage->pixels, w, h, srcPixelsAllMips, gfxImage->pixelFormat );
    
    free( srcPixelsAllMips );

    return true;
}


bool GfxImage_Load( GfxImage* gfxImage, const GfxImageCreateInfo& createInfo )
{
    static_assert( sizeof( GfxImage ) == sizeof( std::string ) + 64, "Don't forget to update this function if added/removed members from GfxImage!" );
    PG_ASSERT( gfxImage );

    gfxImage->name        = createInfo.name;
    gfxImage->imageType   = createInfo.imageType;
    gfxImage->pixelFormat = createInfo.dstPixelFormat;
    gfxImage->depth       = 1;
    gfxImage->mipLevels   = 1;
    gfxImage->numFaces    = 1;

    bool success;
    if ( createInfo.imageType == GfxImageType::TYPE_2D )
    {
        success = Load_GfxImage_2D( gfxImage, createInfo );
    } 
    else
    {
        LOG_ERR( "GfxImageType '%d' not supported yet\n", static_cast< int >( createInfo.imageType ) );
        success = false;
    }

    return success;
}


bool Fastfile_GfxImage_Load( GfxImage* image, Serializer* serializer )
{
    static_assert( sizeof( GfxImage ) == sizeof( std::string ) + 64, "Don't forget to update this function if added/removed members from GfxImage!" );
    
    PG_ASSERT( image && serializer );
    serializer->Read( image->name );
    serializer->Read( reinterpret_cast< char* >( image ) + offsetof( GfxImage, width ), sizeof( GfxImage ) - offsetof( GfxImage, width ) );
    image->pixels = static_cast< unsigned char* >( malloc( image->totalSizeInBytes ) );
    serializer->Read( image->pixels, image->totalSizeInBytes );
    image->textureHandle = GFX_INVALID_TEXTURE_HANDLE;

    return true;
}


bool Fastfile_GfxImage_Save( const GfxImage * const image, Serializer* serializer )
{
    static_assert( sizeof( GfxImage ) == sizeof( std::string ) + 64, "Don't forget to update this function if added/removed members from GfxImage!" );
    
    PG_ASSERT( image && serializer );
    PG_ASSERT( image->pixels );
    serializer->Write( image->name );
    const unsigned char* restOfImage = reinterpret_cast< const unsigned char* >( image );
    serializer->Write( restOfImage + offsetof( GfxImage, width ), sizeof( GfxImage ) - offsetof( GfxImage, width ) );
    serializer->Write( image->pixels, image->totalSizeInBytes );

    return true;
}


int CalculateNumMips( int width, int height )
{
    int largestDim = std::max( width, height );
    return 1 + static_cast< int >( std::log2f( static_cast< float >( largestDim ) ) );
}


size_t CalculateTotalFaceSizeWithMips( int mip0Width, int mip0Height,PixelFormat format )
{
    PG_ASSERT( mip0Width > 0 && mip0Height > 0 );
    int w = mip0Width;
    int h = mip0Height;
    int numMips = CalculateNumMips( w, h );
    int bytesPerPixel = NumBytesPerPixel( format );
    size_t currentSize = 0;
    for ( int mipLevel = 0; mipLevel < numMips; ++mipLevel )
    {
        currentSize += w * h * bytesPerPixel;
        w = std::max( 1, w >> 1 );
        h = std::max( 1, h >> 1 );
    }

    return currentSize;
}


} // namespace Progression
