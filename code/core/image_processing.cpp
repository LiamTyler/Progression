#include "image_processing.hpp"
#include "shared/assert.hpp"
#include "shared/color_spaces.hpp"
#include "shared/logger.hpp"
#include <stdexcept>

namespace PG
{

FloatImage2D CompositeImage( const CompositeImageInput& input )
{
    PG_ASSERT( input.compositeType == CompositeType::REMAP );

    // validate
    if ( input.outputColorSpace != ColorSpace::SRGB && input.outputColorSpace != ColorSpace::LINEAR )
        throw std::runtime_error( "CompositeImage: output color space must be set to SRGB or LINEAR" );
    if ( input.sourceImages.size() == 0 )
        throw std::runtime_error( "CompositeImage: no source images specified" );
    if ( input.sourceImages.size() > 4 )
        throw std::runtime_error( "CompositeImage: too many source images. Only specify up to 4" );

    ImageLoadFlags imgLoadFlags = input.flipVertically ? ImageLoadFlags::FLIP_VERTICALLY : ImageLoadFlags::DEFAULT;

    uint32_t numOutputChannels = 0;
    FloatImage2D sourceImages[4];
    ColorSpace sourceColorSpaces[4];
    uint32_t width  = 0;
    uint32_t height = 0;
    for ( size_t i = 0; i < input.sourceImages.size(); ++i )
    {
        if ( !sourceImages[i].Load( input.sourceImages[i].filename, imgLoadFlags ) )
        {
            throw std::runtime_error( "Failed to load composite source image" );
        }
        sourceColorSpaces[i] = input.sourceImages[i].colorSpace;
        if ( sourceColorSpaces[i] == ColorSpace::INFER )
        {
            sourceColorSpaces[i] = sourceImages[i].numChannels > 2 ? ColorSpace::SRGB : ColorSpace::LINEAR;
        }

        width  = std::max( width, sourceImages[i].width );
        height = std::max( height, sourceImages[i].height );

        for ( const Remap& remap : input.sourceImages[i].remaps )
        {
            numOutputChannels = std::max<uint32_t>( numOutputChannels, Underlying( remap.to ) + 1 );
        }
    }

    ColorSpace outputColorSpace = input.outputColorSpace;
    FloatImage2D outputImg( width, height, numOutputChannels );
    for ( size_t i = 0; i < input.sourceImages.size(); ++i )
    {
        FloatImage2D srcImage = sourceImages[i].Resize( width, height );
        bool convertToLinear  = sourceColorSpaces[i] == ColorSpace::SRGB && outputColorSpace == ColorSpace::LINEAR;
        bool convertToSRGB    = sourceColorSpaces[i] == ColorSpace::LINEAR && outputColorSpace == ColorSpace::SRGB;
        for ( uint32_t pixelIndex = 0; pixelIndex < width * height; ++pixelIndex )
        {
            glm::vec4 pixel = srcImage.GetFloat4( pixelIndex );

            // assume that the alpha channel is always linear, in both src and dst images
            for ( const Remap& remap : input.sourceImages[i].remaps )
            {
                float x = pixel[Underlying( remap.from )];
                if ( remap.from != Channel::A && convertToLinear )
                {
                    x = PG::GammaSRGBToLinear( x );
                }
                if ( remap.to != Channel::A && convertToSRGB )
                {
                    x = PG::LinearToGammaSRGB( x );
                }

                outputImg.data[outputImg.numChannels * pixelIndex + Underlying( remap.to )] = x;
            }
        }
    }

    return outputImg;
}

ImageFormat PixelFormatToImageFormat( PixelFormat pixelFormat )
{
    static_assert( Underlying( ImageFormat::COUNT ) == 27, "don't forget to update this switch statement" );
    static_assert( Underlying( PixelFormat::NUM_PIXEL_FORMATS ) == 85, "don't forget to update the code below" );

    switch ( pixelFormat )
    {
    case PixelFormat::R8_UNORM:
    case PixelFormat::R8_SNORM:
    case PixelFormat::R8_UINT:
    case PixelFormat::R8_SINT:
    case PixelFormat::R8_SRGB: return ImageFormat::R8_UNORM;
    case PixelFormat::R8_G8_UNORM:
    case PixelFormat::R8_G8_SNORM:
    case PixelFormat::R8_G8_UINT:
    case PixelFormat::R8_G8_SINT:
    case PixelFormat::R8_G8_SRGB: return ImageFormat::R8_G8_UNORM;
    case PixelFormat::R8_G8_B8_UNORM:
    case PixelFormat::R8_G8_B8_SNORM:
    case PixelFormat::R8_G8_B8_UINT:
    case PixelFormat::R8_G8_B8_SINT:
    case PixelFormat::R8_G8_B8_SRGB: return ImageFormat::R8_G8_B8_UNORM;
    case PixelFormat::B8_G8_R8_UNORM:
    case PixelFormat::B8_G8_R8_SNORM:
    case PixelFormat::B8_G8_R8_UINT:
    case PixelFormat::B8_G8_R8_SINT:
    case PixelFormat::B8_G8_R8_SRGB: return ImageFormat::R8_G8_B8_UNORM;
    case PixelFormat::R8_G8_B8_A8_UNORM:
    case PixelFormat::R8_G8_B8_A8_SNORM:
    case PixelFormat::R8_G8_B8_A8_UINT:
    case PixelFormat::R8_G8_B8_A8_SINT:
    case PixelFormat::R8_G8_B8_A8_SRGB: return ImageFormat::R8_G8_B8_A8_UNORM;
    case PixelFormat::B8_G8_R8_A8_UNORM:
    case PixelFormat::B8_G8_R8_A8_SNORM:
    case PixelFormat::B8_G8_R8_A8_UINT:
    case PixelFormat::B8_G8_R8_A8_SINT:
    case PixelFormat::B8_G8_R8_A8_SRGB: return ImageFormat::R8_G8_B8_A8_UNORM;
    case PixelFormat::R16_UNORM:
    case PixelFormat::R16_SNORM:
    case PixelFormat::R16_UINT:
    case PixelFormat::R16_SINT: return ImageFormat::R16_UNORM;
    case PixelFormat::R16_FLOAT: return ImageFormat::R16_FLOAT;
    case PixelFormat::R16_G16_UNORM:
    case PixelFormat::R16_G16_SNORM:
    case PixelFormat::R16_G16_UINT:
    case PixelFormat::R16_G16_SINT: return ImageFormat::R16_G16_UNORM;
    case PixelFormat::R16_G16_FLOAT: return ImageFormat::R16_G16_FLOAT;
    case PixelFormat::R16_G16_B16_UNORM:
    case PixelFormat::R16_G16_B16_SNORM:
    case PixelFormat::R16_G16_B16_UINT:
    case PixelFormat::R16_G16_B16_SINT: return ImageFormat::R16_G16_B16_UNORM;
    case PixelFormat::R16_G16_B16_FLOAT: return ImageFormat::R16_G16_B16_FLOAT;
    case PixelFormat::R16_G16_B16_A16_UNORM:
    case PixelFormat::R16_G16_B16_A16_SNORM:
    case PixelFormat::R16_G16_B16_A16_UINT:
    case PixelFormat::R16_G16_B16_A16_SINT: return ImageFormat::R16_G16_B16_A16_UNORM;
    case PixelFormat::R16_G16_B16_A16_FLOAT: return ImageFormat::R16_G16_B16_A16_FLOAT;
    case PixelFormat::R32_FLOAT: return ImageFormat::R32_FLOAT;
    case PixelFormat::R32_G32_FLOAT: return ImageFormat::R32_G32_FLOAT;
    case PixelFormat::R32_G32_B32_FLOAT: return ImageFormat::R32_G32_B32_FLOAT;
    case PixelFormat::R32_G32_B32_A32_FLOAT: return ImageFormat::R32_G32_B32_A32_FLOAT;
    case PixelFormat::DEPTH_16_UNORM: return ImageFormat::R16_UNORM;
    case PixelFormat::DEPTH_32_FLOAT: return ImageFormat::R32_FLOAT;
    case PixelFormat::STENCIL_8_UINT: return ImageFormat::R8_UNORM;

    case PixelFormat::BC1_RGB_UNORM:
    case PixelFormat::BC1_RGB_SRGB:
    case PixelFormat::BC1_RGBA_UNORM:
    case PixelFormat::BC1_RGBA_SRGB: return ImageFormat::BC1_UNORM;
    case PixelFormat::BC2_UNORM:
    case PixelFormat::BC2_SRGB: return ImageFormat::BC2_UNORM;
    case PixelFormat::BC3_UNORM:
    case PixelFormat::BC3_SRGB: return ImageFormat::BC3_UNORM;
    case PixelFormat::BC4_UNORM: return ImageFormat::BC4_UNORM;
    case PixelFormat::BC4_SNORM: return ImageFormat::BC4_SNORM;
    case PixelFormat::BC5_UNORM: return ImageFormat::BC5_UNORM;
    case PixelFormat::BC5_SNORM: return ImageFormat::BC5_SNORM;
    case PixelFormat::BC6H_UFLOAT: return ImageFormat::BC6H_U16F;
    case PixelFormat::BC6H_SFLOAT: return ImageFormat::BC6H_S16F;
    case PixelFormat::BC7_UNORM:
    case PixelFormat::BC7_SRGB: return ImageFormat::BC7_UNORM;
    default: return ImageFormat::INVALID;
    }
}

} // namespace PG
