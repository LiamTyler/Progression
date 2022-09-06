#include "asset/asset_cache.hpp"
#include "asset/types/gfx_image.hpp"
#include "shared/assert.hpp"
#include "shared/filesystem.hpp"
#include "shared/logger.hpp"
#include "shared/serializer.hpp"
#include <cstdio>


static void DisplayHelp()
{
    auto msg =
        "Usage: GfxImageViewer PATH\n"
        "\tPATH must be a valid path to a gfximage .pgi file\n"
        "\tOutputs: will print out the image stats and decompress the image into a [PATH]_f[x]_m[y].png file\n"
        "\t\tfor most formats, except for BC6/float images which will generate EXR image(s)\n"
        "\t\tThe f[x] is the face index, and the m[y] is the mip level\n";
    printf( "%s\n", msg );
}

using namespace PG;

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
    case PixelFormat::R8_SRGB:
        return ImageFormat::R8_UNORM;
    case PixelFormat::R8_G8_UNORM:
    case PixelFormat::R8_G8_SNORM:
    case PixelFormat::R8_G8_UINT:
    case PixelFormat::R8_G8_SINT:
    case PixelFormat::R8_G8_SRGB:
        return ImageFormat::R8_G8_UNORM;
    case PixelFormat::R8_G8_B8_UNORM:
    case PixelFormat::R8_G8_B8_SNORM:
    case PixelFormat::R8_G8_B8_UINT:
    case PixelFormat::R8_G8_B8_SINT:
    case PixelFormat::R8_G8_B8_SRGB:
        return ImageFormat::R8_G8_B8_UNORM;
    case PixelFormat::B8_G8_R8_UNORM:
    case PixelFormat::B8_G8_R8_SNORM:
    case PixelFormat::B8_G8_R8_UINT:
    case PixelFormat::B8_G8_R8_SINT:
    case PixelFormat::B8_G8_R8_SRGB:
        return ImageFormat::R8_G8_B8_UNORM;
    case PixelFormat::R8_G8_B8_A8_UNORM:
    case PixelFormat::R8_G8_B8_A8_SNORM:
    case PixelFormat::R8_G8_B8_A8_UINT:
    case PixelFormat::R8_G8_B8_A8_SINT:
    case PixelFormat::R8_G8_B8_A8_SRGB:
        return ImageFormat::R8_G8_B8_A8_UNORM;
    case PixelFormat::B8_G8_R8_A8_UNORM:
    case PixelFormat::B8_G8_R8_A8_SNORM:
    case PixelFormat::B8_G8_R8_A8_UINT:
    case PixelFormat::B8_G8_R8_A8_SINT:
    case PixelFormat::B8_G8_R8_A8_SRGB:
        return ImageFormat::R8_G8_B8_A8_UNORM;
    case PixelFormat::R16_UNORM:
    case PixelFormat::R16_SNORM:
    case PixelFormat::R16_UINT:
    case PixelFormat::R16_SINT:
        return ImageFormat::R16_UNORM;
    case PixelFormat::R16_FLOAT:
        return ImageFormat::R16_FLOAT;
    case PixelFormat::R16_G16_UNORM:
    case PixelFormat::R16_G16_SNORM:
    case PixelFormat::R16_G16_UINT:
    case PixelFormat::R16_G16_SINT:
        return ImageFormat::R16_G16_UNORM;
    case PixelFormat::R16_G16_FLOAT:
        return ImageFormat::R16_G16_FLOAT;
    case PixelFormat::R16_G16_B16_UNORM:
    case PixelFormat::R16_G16_B16_SNORM:
    case PixelFormat::R16_G16_B16_UINT:
    case PixelFormat::R16_G16_B16_SINT:
        return ImageFormat::R16_G16_B16_UNORM;
    case PixelFormat::R16_G16_B16_FLOAT:
        return ImageFormat::R16_G16_B16_FLOAT;
    case PixelFormat::R16_G16_B16_A16_UNORM:
    case PixelFormat::R16_G16_B16_A16_SNORM:
    case PixelFormat::R16_G16_B16_A16_UINT:
    case PixelFormat::R16_G16_B16_A16_SINT:
        return ImageFormat::R16_G16_B16_A16_UNORM;
    case PixelFormat::R16_G16_B16_A16_FLOAT:
        return ImageFormat::R16_G16_B16_A16_FLOAT;
    case PixelFormat::R32_FLOAT:
        return ImageFormat::R32_FLOAT;
    case PixelFormat::R32_G32_FLOAT:
        return ImageFormat::R32_G32_FLOAT;
    case PixelFormat::R32_G32_B32_FLOAT:
        return ImageFormat::R32_G32_B32_FLOAT;
    case PixelFormat::R32_G32_B32_A32_FLOAT:
        return ImageFormat::R32_G32_B32_A32_FLOAT;
    case PixelFormat::DEPTH_16_UNORM:
        return ImageFormat::R16_UNORM;
    case PixelFormat::DEPTH_32_FLOAT:
        return ImageFormat::R32_FLOAT;
    case PixelFormat::STENCIL_8_UINT:
        return ImageFormat::R8_UNORM;
    case PixelFormat::BC1_RGB_UNORM:
    case PixelFormat::BC1_RGB_SRGB:
    case PixelFormat::BC1_RGBA_UNORM:
    case PixelFormat::BC1_RGBA_SRGB:
        return ImageFormat::BC1_UNORM;
    case PixelFormat::BC2_UNORM:
    case PixelFormat::BC2_SRGB:
        return ImageFormat::BC2_UNORM;
    case PixelFormat::BC3_UNORM:
    case PixelFormat::BC3_SRGB:
        return ImageFormat::BC3_UNORM;
    case PixelFormat::BC4_UNORM:
        return ImageFormat::BC4_UNORM;
    case PixelFormat::BC4_SNORM:
        return ImageFormat::BC4_SNORM;
    case PixelFormat::BC5_UNORM:
        return ImageFormat::BC4_UNORM;
    case PixelFormat::BC5_SNORM:
        return ImageFormat::BC5_SNORM;
    case PixelFormat::BC6H_UFLOAT:
        return ImageFormat::BC6H_U16F;
    case PixelFormat::BC6H_SFLOAT:
        return ImageFormat::BC6H_S16F;
    case PixelFormat::BC7_UNORM:
    case PixelFormat::BC7_SRGB:
        return ImageFormat::BC7_UNORM;
    default:
        return ImageFormat::INVALID;
    }
}

std::string ImageTypeToString( Gfx::ImageType type )
{
    PG_ASSERT( Underlying( type ) <= Underlying( Gfx::ImageType::NUM_IMAGE_TYPES ) );
    static const char* names[] =
    {
        "TYPE_1D",
        "TYPE_1D_ARRAY",
        "TYPE_2D",
        "TYPE_2D_ARRAY",
        "TYPE_CUBEMAP",
        "TYPE_CUBEMAP_ARRAY",
        "TYPE_3D",
    };

    return names[Underlying( type )];
}

int main( int argc, char* argv[] )
{
    if ( argc < 2 )
    {
        DisplayHelp();
        return 0;
    }

    Logger_Init();
    Logger_AddLogLocation( "stdout", stdout );

    Serializer in;
    if ( !in.OpenForRead( argv[1] ) )
    {
        LOG_ERR( "Could not open file '%s'", argv[1] );
        return 0;
    }
    GfxImage image;
    if ( !image.FastfileLoad( &in ) )
    {
        LOG_ERR( "Failed to deserialize image file '%s'", argv[1] );
        return 0;
    }

    LOG( "width: %u", image.width );
    LOG( "height: %u", image.height );
    LOG( "depth: %u", image.depth );
    LOG( "mipLevels: %u", image.mipLevels );
    LOG( "numFaces: %u", image.numFaces );
    LOG( "totalSizeInBytes: %u", image.totalSizeInBytes );
    LOG( "pixelFormat: %s", PixelFormatName( image.pixelFormat ).c_str() );
    LOG( "imageType: %s", ImageTypeToString( image.imageType ).c_str() );

    ImageFormat imgFormat = PixelFormatToImageFormat( image.pixelFormat );
    if ( imgFormat == ImageFormat::INVALID )
    {
        LOG_ERR( "Pixel format is not representible by the RawImage2D ImageFormat. Cannot convert and save image(s)" );
        return 0;
    }
    ImageFormat dstFormat = imgFormat;
    if ( IsFormatBCCompressed( imgFormat ) )
    {
        dstFormat = BCGetFormatAfterDecompression( imgFormat );
    }

    std::string fnamePrefix = GetFilenameMinusExtension( argv[1] );
    for ( uint32_t face = 0; face < image.numFaces; ++face )
    {
        uint32_t w = image.width;
        uint32_t h = image.height;
        for ( uint32_t mipLevel = 0; mipLevel < image.mipLevels; ++mipLevel )
        {
            uint8_t* pixels = image.GetPixels( face, mipLevel );
            RawImage2D img( w, h, imgFormat, pixels );
            if ( dstFormat != imgFormat )
            {
                img = img.Convert( dstFormat );
            }
            std::string fname = fnamePrefix + "_f" + std::to_string( face ) + "_m" + std::to_string( mipLevel );
            fname += IsFormat8BitUnorm( dstFormat ) ? ".png" : ".exr";
            {
                if ( !img.Save( fname ) )
                {
                    return 0;
                }
            }

            w = std::max( 1u, w >> 1 );
            h = std::max( 1u, h >> 1 );
        }
    }

    return 0;
}