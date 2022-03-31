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


template <typename ChannelT>
static inline void TIFFTranscodeStrip( uint32_t channels, ChannelT *cp, uint32_t width, uint32_t height, int fromSkew, int toSkew, uint8_t *stripBuffer )
{
	ChannelT *wp = (ChannelT*)( stripBuffer );
	fromSkew *= channels;
	while ( height-- > 0 )
	{
		for ( uint32_t x = width; x-- > 0; )
		{
			memcpy( cp, wp, sizeof( ChannelT ) * channels );
			cp += channels;
			wp += channels;
		}
		cp += toSkew;
		wp += fromSkew;
	}
}


template <typename ChannelT>
static bool TIFFDecodeStripContigous( TIFF* img, const char *name, ChannelT* raster, uint32_t width, uint32_t height, uint32_t channels )
{
	TIFF* tif = img;
	uint32_t row, y, nrow, nrowsub, rowstoread;
	uint32_t rowsperstrip;
	uint16_t subsamplinghor,subsamplingver;
	uint32_t imagewidth = width;
	tmsize_t scanline;
	int fromSkew, toSkew;
	int ret = 1;

	const tmsize_t stripBufferSize = TIFFStripSize( tif );
	std::unique_ptr<uint8_t[]> stripBuffer( new ( std::nothrow ) uint8_t[stripBufferSize]{} );

	uint16_t orientation;
	TIFFGetField( img, TIFFTAG_ORIENTATION, &orientation );
    y      = 0;
    toSkew = 0;

	uint32_t row_offset = 0;

	TIFFGetFieldDefaulted( tif, TIFFTAG_ROWSPERSTRIP, &rowsperstrip );
	TIFFGetFieldDefaulted( tif, TIFFTAG_YCBCRSUBSAMPLING, &subsamplinghor, &subsamplingver );
	scanline = TIFFScanlineSize( tif );

	fromSkew = (width < imagewidth ? imagewidth - width : 0 );

	for ( row = 0; row < height; row += nrow )
	{
		rowstoread = rowsperstrip - (row + row_offset) % rowsperstrip;
		nrow = (row + rowstoread > height ? height - row : rowstoread);
		nrowsub = nrow;
		if ( ( nrowsub % subsamplingver ) !=0 )
			nrowsub += subsamplingver - nrowsub % subsamplingver;

		uint32_t stripPosition = TIFFComputeStrip( tif, row + row_offset, 0 );
		tmsize_t stripSize = ( ( row + row_offset ) % rowsperstrip + nrowsub ) * scanline;

		if ( TIFFReadEncodedStrip( tif, stripPosition, stripBuffer.get(), stripSize ) == ( tmsize_t )( -1 ) )
		{
			return false;
		}

		TIFFTranscodeStrip( channels, raster + y * width * channels, width, nrow, fromSkew, toSkew, stripBuffer.get() );

		y += nrow;
	}

	return true;
}


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