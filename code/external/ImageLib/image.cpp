#include "image.hpp"
#include "image_load.hpp"
#include "image_save.hpp"
#include "shared/assert.hpp"
#include "shared/filesystem.hpp"
#include "shared/float_conversions.hpp"
#include "shared/logger.hpp"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb/stb_image_resize.h"
#include "tinyexr/tinyexr.h"
#include <climits>
#include <memory>
#include <vector>

static glm::u8vec4  DEFAULT_PIXEL_BYTE  = glm::u8vec4( 0, 0, 0, 255 );
static glm::u16vec4 DEFAULT_PIXEL_SHORT = glm::u16vec4( 0, 0, 0, USHRT_MAX );
static glm::vec4    DEFAULT_PIXEL_FLOAT = glm::vec4( 0, 0, 0, 1 );

constexpr bool IsFormat8BitUnorm( RawImage2D::Format format )
{
    return Underlying( RawImage2D::Format::R8_UNORM ) <= Underlying( format ) && Underlying( format ) <= Underlying( RawImage2D::Format::R8_G8_B8_A8_UNORM );
}

constexpr bool IsFormat16BitUnorm( RawImage2D::Format format )
{
    return Underlying( RawImage2D::Format::R16_UNORM ) <= Underlying( format ) && Underlying( format ) <= Underlying( RawImage2D::Format::R16_G16_B16_A16_UNORM );
}

constexpr bool IsFormat16BitFloat( RawImage2D::Format format )
{
    return Underlying( RawImage2D::Format::R16_FLOAT ) <= Underlying( format ) && Underlying( format ) <= Underlying( RawImage2D::Format::R16_G16_B16_A16_FLOAT );
}

constexpr bool IsFormat32BitFloat( RawImage2D::Format format )
{
    return Underlying( RawImage2D::Format::R32_FLOAT ) <= Underlying( format ) && Underlying( format ) <= Underlying( RawImage2D::Format::R32_G32_B32_A32_FLOAT );
}


uint8_t* RawImage2D::GetCompressedBlock( int blockX, int blockY )
{
    uint32_t numBlocksX = (width + 3) / 4;
    size_t offset = 16 * BitsPerPixel() / 8 * (blockY * numBlocksX + blockX);
    return &data[offset];
}


template <typename T, std::underlying_type_t<RawImage2D::Format> baseFormat>
void GetBlockClamped8BitConvert( uint32_t blockX, uint32_t blockY, uint32_t width, uint32_t height, uint32_t numChannels, uint8_t* output, const T* input )
{
    for ( uint32_t r = 0; r < 4; ++r )
    {
        uint32_t row = std::min( height - 1, 4 * blockY + r );
        for ( uint32_t c = 0; c < 4; ++c )
        {
            uint32_t col = std::min( width - 1, 4 * blockX + c );

            uint32_t srcOffset = numChannels * (row * width + col);
            uint32_t dstOffset = 4 * (r * 4 + c);
            for ( uint32_t channel = 0; channel < 4; ++channel )
            {
                if ( channel < numChannels )
                {
                    if constexpr ( baseFormat == Underlying( RawImage2D::Format::R8_UNORM ) )
                    {
                        output[dstOffset + channel] = input[srcOffset + channel];
                    }
                    else if constexpr ( baseFormat == Underlying( RawImage2D::Format::R16_UNORM ) )
                    {
                        output[dstOffset + channel] = UNormFloatToByte( UNorm16ToFloat( input[srcOffset + channel] ) );
                    }
                    else if constexpr ( baseFormat == Underlying( RawImage2D::Format::R16_FLOAT ) )
                    {
                        output[dstOffset + channel] = UNormFloatToByte( Float16ToFloat32( input[srcOffset + channel] ) );
                    }
                    else
                    {
                        output[dstOffset + channel] = UNormFloatToByte( input[srcOffset + channel] );
                    }
                }
                else
                {
                    output[dstOffset + channel] = DEFAULT_PIXEL_BYTE[channel];
                }
                
            }
        }
    }
}


void RawImage2D::GetBlockClamped8Bit( int blockX, int blockY, uint8_t* output ) const
{
    uint32_t numChannels = NumChannels();
    if ( IsFormat8BitUnorm( format ) )
    {
        GetBlockClamped8BitConvert<uint8_t, Underlying( Format::R8_UNORM )>( (uint32_t)blockX, (uint32_t)blockY, width, height, numChannels, output, Raw<uint8_t>() );
    }
    else if ( IsFormat16BitUnorm( format ) )
    {
        GetBlockClamped8BitConvert<uint16_t, Underlying( Format::R16_UNORM )>( (uint32_t)blockX, (uint32_t)blockY, width, height, numChannels, output, Raw<uint16_t>() );
    }
    else if ( IsFormat16BitFloat( format ) )
    {
        GetBlockClamped8BitConvert<float16, Underlying( Format::R16_FLOAT )>( (uint32_t)blockX, (uint32_t)blockY, width, height, numChannels, output, Raw<float16>() );
    }
    else if ( IsFormat32BitFloat( format ) )
    {
        GetBlockClamped8BitConvert<float, Underlying( Format::R32_FLOAT )>( (uint32_t)blockX, (uint32_t)blockY, width, height, numChannels, output, Raw<float>() );
    }
    else
    {
        memset( output, 0, 64 );
    }
}


float RawImage2D::GetPixelAsFloat( int row, int col, int chan ) const
{
    uint32_t numChannels = NumChannels();
    if ( chan >= (int)numChannels )
    {
        return DEFAULT_PIXEL_FLOAT[chan];
    
    }
    int index = numChannels * (row * width + col) + chan;

    if ( IsFormat8BitUnorm( format ) )
    {
        return UNormByteToFloat( Raw<uint8_t>()[index] );
    }
    else if ( IsFormat16BitUnorm( format ) )
    {
        return UNorm16ToFloat( Raw<uint16_t>()[index] );
    }
    else if ( IsFormat16BitFloat( format ) )
    {
        return Float16ToFloat32( Raw<float16>()[index] );
    }
    else if ( IsFormat32BitFloat( format ) )
    {
        return Raw<float>()[index];
    }

    return 0;
}


glm::vec4 RawImage2D::GetPixelAsFloat4( int row, int col ) const
{
    glm::vec4 pixel = DEFAULT_PIXEL_FLOAT;
    uint32_t numChannels = NumChannels();
    for ( uint32_t chan = 0; chan < numChannels; ++chan )
    {
        pixel[chan] = GetPixelAsFloat( row, col, chan );
    }

    return pixel;
}


void RawImage2D::SetPixelFromFloat( int row, int col, int chan, float x )
{
    uint32_t numChannels = NumChannels();
    int index = numChannels * (row * width + col) + chan;
    if ( IsFormat8BitUnorm( format ) )
    {
        Raw<uint8_t>()[index] = UNormFloatToByte( x );
    }
    else if ( IsFormat16BitUnorm( format ) )
    {
        Raw<uint16_t>()[index] = FloatToUNorm16( x );
    }
    else if ( IsFormat16BitFloat( format ) )
    {
        Raw<float16>()[index] = Float32ToFloat16( x );
    }
    else if ( IsFormat32BitFloat( format ) )
    {
        Raw<float>()[index] = x;
    }
}


void RawImage2D::SetPixelFromFloat4( int row, int col, glm::vec4 pixel )
{
}


// TODO: optimize
RawImage2D RawImage2D::Convert( Format dstFormat ) const
{
    RawImage2D outputImg( width, height, dstFormat );
    uint32_t inputChannels = NumChannels();
    uint32_t outputChannels = outputImg.NumChannels();

    for ( uint32_t row = 0; row < height; ++row )
    {
        for ( uint32_t col = 0; col < width; ++col )
        {
            glm::vec4 pixelAsFloat = GetPixelAsFloat4( row, col );
            
            {
                if ( IsFormat8BitUnorm( dstFormat ) )
                {
                    for ( uint32_t chan = 0; chan < outputChannels; ++chan )
                }
                else if ( IsFormat16BitUnorm( dstFormat ) )
                {
                }
                else if ( IsFormat16BitFloat( dstFormat ) )
                {
                }
                else if ( IsFormat32BitFloat( dstFormat ) ) 
            }
        }
    }

    return outputImg;
}