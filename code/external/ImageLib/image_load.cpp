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

/*
#include "image_load.hpp"
#include "shared/assert.hpp"
#include "shared/filesystem.hpp"
#include "shared/float_conversions.hpp"
#include "shared/logger.hpp"
#define STBI_NO_PIC
#define STBI_NO_PSD   
#define STBI_NO_GIF   
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "tiffio.h"
#include "tinyexr/tinyexr.h"
#include <memory>
#include <vector>

glm::u8vec4* Load2D_U8( const std::string& filename, int& width, int& height )
{
    glm::u8vec4* pixels = nullptr;
    bool loadSuccessful = true;

    std::string ext = GetFileExtension( filename );
    if ( ext == ".jpg" || ext == ".png" || ext == ".tga" || ext == ".bmp" || ext == ".ppm" || ext == ".pbm" )
    {
        int numChannels;
        pixels = reinterpret_cast<glm::u8vec4*>( stbi_load( filename.c_str(), &width, &height, &numChannels, STBI_rgb_alpha ) );
        loadSuccessful = pixels != nullptr;
    }
    else if ( ext == ".tif" || ext == ".tiff" )
    {
        TIFFSetWarningHandler( NULL );
	    TIFFSetWarningHandlerExt( NULL );
	    TIFFSetErrorHandler( NULL );
	    TIFFSetErrorHandlerExt( NULL );

        TIFF* tiffr = TIFFOpen( filename.c_str(), "r" );
        if ( tiffr )
		{
			TIFFGetField( tiffr, TIFFTAG_IMAGEWIDTH, &width );
	        TIFFGetField( tiffr, TIFFTAG_IMAGELENGTH, &height );

            pixels = static_cast<glm::u8vec4*>( malloc( width * height * 4 ) );
            loadSuccessful = TIFFReadRGBAImageOriented( tiffr, width, height, (uint32_t*)pixels, ORIENTATION_TOPLEFT, 0 ) == 1;
            
            TIFFClose( tiffr );
        }
    }
    else
    {
        LOG_ERR( "Image filetype '%s' for image '%s' is not supported", ext.c_str(), filename.c_str() );
        return nullptr;
    }

    if ( !loadSuccessful )
    {
        free( pixels );
        return nullptr;
    }

    return pixels;
}


glm::vec4* Load2D_F32( const std::string& filename, int& width, int& height )
{
    glm::vec4* pixels = nullptr;
    bool loadSuccessful = true;

    std::string ext = GetFileExtension( filename );
    if ( ext == ".jpg" || ext == ".png" || ext == ".tga" || ext == ".bmp" || ext == ".ppm" || ext == ".pbm" )
    {
        stbi_ldr_to_hdr_gamma( 1.0f );
        stbi_ldr_to_hdr_scale( 1.0f );
        int numChannels;
        pixels = reinterpret_cast<glm::vec4*>( stbi_loadf( filename.c_str(), &width, &height, &numChannels, STBI_rgb_alpha ) );
        loadSuccessful = pixels != nullptr;
    }
    else if ( ext == ".tif" || ext == ".tiff" )
    {
        TIFFSetWarningHandler( NULL );
	    TIFFSetWarningHandlerExt( NULL );
	    TIFFSetErrorHandler( NULL );
	    TIFFSetErrorHandlerExt( NULL );

        TIFF* tiffr = TIFFOpen( filename.c_str(), "r" );
        if ( tiffr )
		{
			TIFFGetField( tiffr, TIFFTAG_IMAGEWIDTH, &width );
	        TIFFGetField( tiffr, TIFFTAG_IMAGELENGTH, &height );
            // int numChannels = 0;
            // int numBitsPerChannel = 0;
	        // TIFFGetField( tiffr, TIFFTAG_SAMPLESPERPIXEL, &numChannels );
	        // TIFFGetField( tiffr, TIFFTAG_BITSPERSAMPLE, &numBitsPerChannel );
            // if ( numBitsPerChannel != 8 )
            // {
            //     LOG_ERR( "Can't read image with %d bits per channel, image '%s'", numBitsPerChannel, filename.c_str() );
            //     return nullptr;
            // }
            pixels = static_cast<glm::vec4*>( malloc( width * height * sizeof( glm::vec4 ) ) );
            std::vector<uint8_t> tmpBuffer( width * height * 4 );
            loadSuccessful = 1 == TIFFReadRGBAImageOriented( tiffr, width, height, (uint32_t*)tmpBuffer.data(), ORIENTATION_TOPLEFT, 0 );
            for ( int i = 0; i < width * height; ++i )
            {
                for ( int channel = 0; channel < 4; ++channel )
                {
                    float x = tmpBuffer[4*i + channel] / 255.0f;                            
                    pixels[i][channel] = x;
                }
            }
            TIFFClose( tiffr );
        }
    }
    else if ( ext == ".hdr" )
    {
        stbi_ldr_to_hdr_gamma( 1.0f );
        stbi_ldr_to_hdr_scale( 1.0f );
        int numChannels;
        pixels = reinterpret_cast<glm::vec4*>( stbi_loadf( filename.c_str(), &width, &height, &numChannels, STBI_rgb_alpha ) );
        loadSuccessful = pixels != nullptr;
    }
    else if ( ext == ".exr" )
    {
        const char* err = nullptr;
        loadSuccessful = LoadEXR( reinterpret_cast<float**>( &pixels ), &width, &height, filename.c_str(), &err ) == TINYEXR_SUCCESS;
        if ( err )
        {
            LOG_ERR( "Tinyexr error '%s'", err );
        }
    }
    else
    {
        LOG_ERR( "Image filetype '%s' for image '%s' is not supported", ext.c_str(), filename.c_str() );
        return nullptr;
    }

    if ( !loadSuccessful )
    {
        free( pixels );
        return nullptr;
    }

    return pixels;
}


glm::u16vec4* Load2D_F16( const std::string& filename, int& width, int& height )
{
    glm::vec4* pixels32 = Load2D_F32( filename, width, height );
    if ( !pixels32 )
    {
        return nullptr;
    }

    glm::u16vec4* pixels = static_cast<glm::u16vec4*>( malloc( width * height * sizeof( uint16_t ) ) );
    for ( int i = 0; i < width * height; ++i )
    {
        pixels[i] = Float32ToFloat16( pixels32[i] );
    }
    free( pixels32 );
    return pixels;
}
*/

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