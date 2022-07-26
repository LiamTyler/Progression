#include "image.hpp"
#include "shared/filesystem.hpp"
#include "shared/logger.hpp"
#define STBI_NO_PIC
#define STBI_NO_PSD   
#define STBI_NO_GIF   
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "tiffio.h"
#include "tinyexr/tinyexr.h"


bool RawImage2D::Load( const std::string& filename )
{
    std::string ext = GetFileExtension( filename );
    if ( ext == ".jpg" || ext == ".png" || ext == ".tga" || ext == ".bmp" || ext == ".ppm" || ext == ".pbm" || ext == ".hdr" )
    {
        FILE* file = fopen( filename.c_str(), "rb" );
        if ( file == NULL )
        {
            LOG_ERR( "RawImage2D::Load: Could not open file '%s'", filename.c_str() );
            return false;
        }

        int w, h, numChannels;
        ImageFormat startFormat;
        uint8_t* pixels;
        if ( ext == ".hdr" )
        {
            startFormat = ImageFormat::R32_FLOAT;
            pixels = (uint8_t*)stbi_loadf( filename.c_str(), &w, &h, &numChannels, 0 );
        }
        else if ( stbi_is_16_bit_from_file( file ) )
        {
            startFormat = ImageFormat::R16_UNORM;
            pixels = (uint8_t*)stbi_load_from_file_16( file, &w, &h, &numChannels, 0 );
        }
        else
        {
            startFormat = ImageFormat::R8_UNORM;
            pixels = stbi_load_from_file( file, &w, &h, &numChannels, 0 );
        }
        if ( !pixels )
        {
            LOG_ERR( "RawImage2D::Load: error while loading image '%s'", filename.c_str() );
            return false;
        }
        data = std::shared_ptr<uint8_t[]>( pixels, []( void* p ) { stbi_image_free( p ); } );
        width = static_cast<uint32_t>( w );
        height = static_cast<uint32_t>( h );
        format = static_cast<ImageFormat>( (int)startFormat + numChannels - 1 );
    }
    else if ( ext == ".exr" )
    {
        const char* err = nullptr;
        float* pixels;
        int w, h;
        bool success = LoadEXR( &pixels, &w, &h, filename.c_str(), &err ) == TINYEXR_SUCCESS;
        if ( !success )
        {
            LOG_ERR( "RawImage2D::Load: error while loading image '%s'", filename.c_str() );
            if ( err )
            {
                LOG_ERR( "\tTinyexr error '%s'", err );
            }
        }
        data = std::shared_ptr<uint8_t[]>( (uint8_t*)pixels, []( void* p ) { free( p ); } );
        width = static_cast<uint32_t>( w );
        height = static_cast<uint32_t>( h );
        format = ImageFormat::R32_G32_B32_A32_FLOAT;
    }
    else
    {
        LOG_ERR( "Image filetype '%s' for image '%s' is not supported", ext.c_str(), filename.c_str() );
        return false;
    }

    return true;
}