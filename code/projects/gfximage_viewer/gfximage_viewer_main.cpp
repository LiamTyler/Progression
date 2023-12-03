#include "asset/asset_cache.hpp"
#include "asset/types/gfx_image.hpp"
#include "bc_compression.hpp"
#include "core/image_processing.hpp"
#include "shared/assert.hpp"
#include "shared/filesystem.hpp"
#include "shared/logger.hpp"
#include "shared/serializer.hpp"
#include <cstdio>

static void DisplayHelp()
{
    auto msg = "Usage: GfxImageViewer PATH\n"
               "\tPATH must be a valid path to a gfximage .pgi file\n"
               "\tOutputs: will print out the image stats and decompress the image into a [PATH]_f[x]_m[y].png file\n"
               "\t\tfor most formats, except for BC6/float images which will generate EXR image(s)\n"
               "\t\tThe f[x] is the face index, and the m[y] is the mip level\n";
    printf( "%s\n", msg );
}

using namespace PG;

std::string ImageTypeToString( ImageType type )
{
    PG_ASSERT( Underlying( type ) <= Underlying( ImageType::NUM_IMAGE_TYPES ) );
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
        dstFormat = GetFormatAfterDecompression( imgFormat );
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

            w = Max( 1u, w >> 1 );
            h = Max( 1u, h >> 1 );
        }
    }

    return 0;
}
