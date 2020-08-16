#include "gfx_image.hpp"
#include "assert.hpp"
#include "image.hpp"
#include "utils/logger.hpp"
#include "utils/cpu_profile.hpp"
#include "glm/glm.hpp"
#include <algorithm>

#include "stb/stb_image_write.h"

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


static void ConvertRGBA32Float( unsigned char* outputImage, int width, int height, const glm::vec4* pixels, PixelFormat format )
{
    PG_ASSERT( pixels );
    PG_ASSERT( outputImage );
    PG_ASSERT( format != PixelFormat::INVALID && format != PixelFormat::NUM_PIXEL_FORMATS );
    PG_ASSERT( !PixelFormatHasDepthFormat( format ) );
    PG_ASSERT( !PixelFormatIsCompressed( format ), "Compression not supported yet" );
    
    PG_PROFILE_START( ConvertRGBA32Float );
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
            glm::vec4 pixel = pixels[row * width + col];
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
    PG_PROFILE_END( ConvertRGBA32Float );
}

bool Load_GfxImage( GfxImage* gfxImage, const GfxImageCreateInfo& createInfo )
{
    PG_ASSERT( gfxImage );

    gfxImage->name        = createInfo.name;
    gfxImage->imageType   = createInfo.imageType;
    gfxImage->pixelFormat = createInfo.dstPixelFormat;
    gfxImage->depth       = 1;
    gfxImage->mipLevels   = 1;
    gfxImage->numFaces    = 1;

    if ( createInfo.imageType == GfxImageType::TYPE_2D )
    {
        ImageCreateInfo srcImgCreateInfo;
        srcImgCreateInfo.filename = createInfo.filename;
        Image srcImage;
        if ( !srcImage.Load( &srcImgCreateInfo ) )
        {
            return false;
        }
        gfxImage->width  = srcImage.width;
        gfxImage->height = srcImage.height;
        size_t imageSize = gfxImage->width * gfxImage->height * NumBytesPerPixel( gfxImage->pixelFormat );
        gfxImage->pixels = static_cast< unsigned char* >( malloc( imageSize ) );
        ConvertRGBA32Float( gfxImage->pixels, gfxImage->width, gfxImage->height, srcImage.pixels, gfxImage->pixelFormat );
    }
    else
    {
        LOG_ERR( "GfxImageType '%d' not supported yet\n", static_cast< int >( createInfo.imageType ) );
        return false;
    }

    return true;
}


} // namespace Progression
