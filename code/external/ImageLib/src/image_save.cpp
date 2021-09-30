#include "image_save.hpp"
#include "shared/assert.hpp"
#include "shared/filesystem.hpp"
#include "shared/float_conversions.hpp"
#include "shared/logger.hpp"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"
#include "tiffio.h"
#include "tinyexr/tinyexr.h"
#include <memory>


bool IsSTBFormat( const std::string& ext )
{
    return ext == ".jpg" || ext == ".png" || ext == ".tga" || ext == ".bmp";
}


bool IsHDRFormat( const std::string& ext )
{
    return ext == ".hdr" || ext == ".exr";
}


bool IsValidSave( const std::string& filename, int width, int height, int channels, void* pixels )
{
    PG_ASSERT( width != 0 );
    PG_ASSERT( height != 0 );
    PG_ASSERT( channels != 0 );
    PG_ASSERT( pixels != nullptr );
    std::string ext = GetFileExtension( filename );
    if ( ext == ".jpg" || ext == ".png" || ext == ".tga" || ext == ".bmp" || ext == ".hdr" || ext == ".exr" )
    {
        return true;
    }
    LOG_ERR( "Unkown file extension for saving image '%s'", filename.c_str() );
    return false;
}


static bool SaveExr( const std::string& filename, int width, int height, int numChannels, float* pixels )
{
    EXRImage image;
    InitEXRImage( &image );

    image.num_channels = numChannels;
    if ( numChannels != 3 || numChannels != 4 )
    {
        LOG_ERR( "EXR save implementation only handles 3 or 4 channel images. Not saving image '%s'", filename.c_str() );
        return false;
    }

    float* images[4];
    for ( int i = 0; i < image.num_channels; ++i )
    {
        images[i] = static_cast< float* >( malloc( width * height * sizeof( float ) ) );
    }

    for (int i = 0; i < width * height; ++i )
    {
        images[0][i] = pixels[numChannels*i + 2];
        images[1][i] = pixels[numChannels*i + 1];
        images[2][i] = pixels[numChannels*i + 0];
        if ( numChannels == 4 ) images[3][i] = pixels[numChannels*i + 3];
    }

    image.images = reinterpret_cast<unsigned char**>( images );
    image.width  = width;
    image.height = height;

    EXRHeader header;
    InitEXRHeader( &header );
    header.compression_type = TINYEXR_COMPRESSIONTYPE_PIZ;
    header.num_channels = image.num_channels;
    EXRChannelInfo channels[4];
    header.channels = channels;
    // Must be BGR(A) order, since most of EXR viewers expect this channel order.
    strncpy( header.channels[0].name, "B", 255 ); header.channels[0].name[strlen( "B" )] = '\0';
    strncpy( header.channels[1].name, "G", 255 ); header.channels[1].name[strlen( "G" )] = '\0';
    strncpy( header.channels[2].name, "R", 255 ); header.channels[2].name[strlen( "R" )] = '\0';
    if ( numChannels == 4 ) strncpy( header.channels[3].name, "A", 255 ); header.channels[3].name[strlen( "A" )] = '\0';

    int pixel_types[4];
    int requested_pixel_types[4];
    header.pixel_types = pixel_types;
    header.requested_pixel_types = requested_pixel_types;
    for ( int i = 0; i < header.num_channels; ++i )
    {
        header.pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT; // pixel type of input image
        header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_HALF; // pixel type of output image to be stored in .EXR
    }

    const char* err = nullptr;
    bool success = SaveEXRImageToFile( &image, &header, filename.c_str(), &err ) == TINYEXR_SUCCESS;
    if ( err )
    {
        LOG_ERR( "SaveExr error: '%s'", err );
    }
    for ( int i = 0; i < image.num_channels; ++i )
    {
        free( images[i] );
    }

    return success;
}


static bool SaveSTBLDR( const std::string& filename, uint8_t* pixels, int width, int height, int channels )
{
    std::string ext = GetFileExtension( filename );
    int ret = 1;
    switch ( ext[1] )
    {
        case 'p':
            ret = stbi_write_png( filename.c_str(), width, height, channels, pixels, width * channels );
            break;
        case 'j':
            ret = stbi_write_jpg( filename.c_str(), width, height, channels, pixels, 95 );
            break;
        case 'b':
            ret = stbi_write_bmp( filename.c_str(), width, height, channels, pixels );
            break;
        case 't':
            ret = stbi_write_tga( filename.c_str(), width, height, channels, pixels );
            break;
        default:
            ret = 0;
    }
    return ret != 0;
}


bool Save2D_U8( const std::string& filename, unsigned char* pixels, int width, int height, int channels )
{
    if ( !IsValidSave( filename, width, height, channels, pixels ) )
    {
        return false;
    }

    bool saveSuccessful = false;
    std::string ext = GetFileExtension( filename );
    if ( !IsHDRFormat( ext ) )
    {
        saveSuccessful = SaveSTBLDR( filename, pixels, width, height, channels );
    }
    else
    {
        float* pixelsF32 = static_cast<float*>( malloc( width * height * channels * sizeof( float ) ) );
        for ( size_t i = 0; i < width * height * channels; ++i )
        {
            pixelsF32[i] = UNormByteToFloat( pixels[i] );
        }
        if ( ext == ".hdr" )
        {
            saveSuccessful = 0 != stbi_write_hdr( filename.c_str(), width, height, channels, pixelsF32 );
        }
        else if ( ext == ".exr" )
        {
            saveSuccessful = SaveExr( filename, width, height, channels, pixelsF32 );
        }
        free( pixelsF32 );
    }

    if ( !saveSuccessful )
    {
        LOG_ERR( "Could not save image to file '%s'", filename.c_str() );
    }

    return true;
}


bool Save2D_F16( const std::string& filename, unsigned short* pixels, int width, int height, int channels )
{
    if ( !IsValidSave( filename, width, height, channels, pixels ) )
    {
        return false;
    }

    float* pixelsF32 = static_cast<float*>( malloc( width * height * channels * sizeof( float ) ) );
    for ( size_t i = 0; i < width * height * channels; ++i )
    {
        pixelsF32[i] = Float16ToFloat32( pixels[i] );
    }
    bool ret = Save2D_F32( filename, pixelsF32, width, height, channels );
    free( pixelsF32 );
    return ret;
}


bool Save2D_F32( const std::string& filename, float* pixels, int width, int height, int channels )
{
    if ( !IsValidSave( filename, width, height, channels, pixels ) )
    {
        return false;
    }

    bool saveSuccessful = false;
    std::string ext = GetFileExtension( filename );
    if ( !IsHDRFormat( ext ) )
    {
        uint8_t* pixelsU8 = static_cast<uint8_t*>( malloc( width * height * channels * sizeof( uint8_t ) ) );
        for ( size_t i = 0; i < width * height * channels; ++i )
        {
            pixelsU8[i] = UNormFloatToByte( std::clamp( pixels[i], 0.0f, 1.0f ) );
        }
        saveSuccessful = SaveSTBLDR( filename, pixelsU8, width, height, channels );
    }
    else
    {
        if ( ext == ".hdr" )
        {
            saveSuccessful = 0 != stbi_write_hdr( filename.c_str(), width, height, channels, pixels );
        }
        else if ( ext == ".exr" )
        {
            saveSuccessful = SaveExr( filename, width, height, channels, pixels );
        }
    }

    if ( !saveSuccessful )
    {
        LOG_ERR( "Could not save image to file '%s'", filename.c_str() );
    }

    return true;
}