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


bool IsValidSave( const std::string& filename, int width, int height, int channels, void* pixels )
{
    PG_ASSERT( width != 0 );
    PG_ASSERT( height != 0 );
    PG_ASSERT( channels != 0 );
    PG_ASSERT( pixels != nullptr );
    std::string ext = GetFileExtension( filename );
    if ( ext == ".jpg" || ext == ".png" || ext == ".tga" || ext == ".bmp" || ext == ".hdr" || ext == ".exr" || ext == ".tif" || ext == ".tiff" )
    {
        return true;
    }
    LOG_ERR( "Unkown file extension for saving image '%s'", filename.c_str() );
    return false;
}


static bool SaveTIFF( const std::string& filename, int width, int height, int numChannels, int bytesPerChannel, void* pixels )
{
    TIFF* output = TIFFOpen( filename.c_str(), "w" );
    if ( !output )
    {
        return false;
    }

    TIFFSetField( output, TIFFTAG_IMAGEWIDTH, width );
	TIFFSetField( output, TIFFTAG_IMAGELENGTH, height );
	TIFFSetField( output, TIFFTAG_SAMPLESPERPIXEL, numChannels );
	TIFFSetField( output, TIFFTAG_BITSPERSAMPLE, bytesPerChannel * 8 );
    TIFFSetField( output, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT );
    TIFFSetField( output, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG );
    TIFFSetField( output, TIFFTAG_ROWSPERSTRIP, 1 );   
    TIFFSetField( output, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB );
    TIFFSetField( output, TIFFTAG_SAMPLEFORMAT, bytesPerChannel == 1 ? SAMPLEFORMAT_UINT : SAMPLEFORMAT_IEEEFP );
    TIFFSetField( output, TIFFTAG_COMPRESSION, COMPRESSION_LZW );
    if ( numChannels == 4 )
    {
        const uint16_t extras[] = { EXTRASAMPLE_UNASSALPHA };
        TIFFSetField( output, TIFFTAG_EXTRASAMPLES, 1, extras );
    }

    char* buf = reinterpret_cast<char*>( pixels );
    int bytesPerPixel = numChannels * bytesPerChannel;
	for ( int row = 0; row < height; row++ )
	{
		if ( TIFFWriteScanline( output, buf + bytesPerPixel * row * width, row ) < 0 )
		{
            TIFFClose( output );
			return false;
		}
	}
        
	TIFFClose( output );
    return true;
}


static bool SaveExr( const std::string& filename, int width, int height, int numChannels, float* pixels )
{
    //EXRHeader header;
    //InitEXRHeader( &header );
    //header.compression_type = TINYEXR_COMPRESSIONTYPE_ZIP;
    //
    //EXRImage image;
    //InitEXRImage( &image );
    //
    //image.num_channels = numChannels;
    //
    //std::vector<float> images[4];
    //images[0].resize( width * height );
    //images[1].resize( width * height );
    //images[2].resize( width * height );
    //if ( numChannels == 4 ) images[3].resize( width * height );
    //
    //for ( int i = 0; i < width * height; ++i )
    //{
    //    images[0][i] = pixels[4*i+0];
    //    images[1][i] = pixels[4*i+1];
    //    images[2][i] = pixels[4*i+2];
    //    if ( numChannels == 4 ) images[3][i] = pixels[4*i+3];
    //}
    //
    //float* image_ptr[4];
    //if ( numChannels == 4 )
    //{
    //    image_ptr[0] = images[3].data(); // A
    //    image_ptr[1] = images[2].data(); // B
    //    image_ptr[2] = images[1].data(); // G
    //    image_ptr[3] = images[0].data(); // R
    //}
    //else
    //{
    //    image_ptr[0] = images[2].data(); // B
    //    image_ptr[1] = images[1].data(); // G
    //    image_ptr[2] = images[0].data(); // R
    //}
    //
    //image.images = (unsigned char**)image_ptr;
    //image.width = width;
    //image.height = height;
    //
    //header.num_channels = numChannels;
    //header.channels = (EXRChannelInfo *)malloc(sizeof(EXRChannelInfo) * header.num_channels); 
    //// From tinyexr: Must be BGR(A) order, since most of EXR viewers expect this channel order.
    //// NOTE! Seems like it has to be (A)BGR instead
    //if ( numChannels == 4 )
    //{
    //    header.channels[0].name[0] = 'A'; header.channels[0].name[1] = '\0';
    //    header.channels[1].name[0] = 'B'; header.channels[1].name[1] = '\0';
    //    header.channels[2].name[0] = 'G'; header.channels[2].name[1] = '\0';
    //    header.channels[3].name[0] = 'R'; header.channels[3].name[1] = '\0';
    //}
    //
    //header.pixel_types = (int*)malloc( sizeof( int ) * header.num_channels ); 
    //header.requested_pixel_types = (int*)malloc( sizeof( int ) * header.num_channels );
    //for ( int i = 0; i < header.num_channels; i++ )
    //{
    //    header.pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT; // pixel type of input image
    //    header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_HALF; // pixel type of output image to be stored in .EXR
    //}
    //
    //const char* err;
    //int ret = SaveEXRImageToFile( &image, &header, filename.c_str(), &err );
    //free( header.channels );
    //free( header.pixel_types );
    //free( header.requested_pixel_types );

    const char* err;
    int ret = SaveEXR( pixels, width, height, numChannels, 1, filename.c_str(), &err );

    if ( ret != TINYEXR_SUCCESS )
    {
        LOG_ERR( "SaveExr error: '%s'", err );
        return false;
    }

    return true;
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
    if ( ext == ".tif" || ext == ".tiff" )
    {
        saveSuccessful = SaveTIFF( filename, width, height, channels, 1, pixels );
    }
    else if ( IsSTBFormat( ext ) )
    {
        saveSuccessful = SaveSTBLDR( filename, pixels, width, height, channels );
    }
    else if ( ext == ".hdr" || ext == ".exr" )
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
    if ( ext == ".tif" || ext == ".tiff" )
    {
        saveSuccessful = SaveTIFF( filename, width, height, channels, 4, pixels );
    }
    else if ( IsSTBFormat( ext ) )
    {
        uint8_t* pixelsU8 = static_cast<uint8_t*>( malloc( width * height * channels * sizeof( uint8_t ) ) );
        for ( size_t i = 0; i < width * height * channels; ++i )
        {
            pixelsU8[i] = UNormFloatToByte( std::clamp( pixels[i], 0.0f, 1.0f ) );
        }
        saveSuccessful = SaveSTBLDR( filename, pixelsU8, width, height, channels );
    }
    else if ( ext == ".hdr" )
    {
        saveSuccessful = 0 != stbi_write_hdr( filename.c_str(), width, height, channels, pixels );
    }
    else if ( ext == ".exr" )
    {
        saveSuccessful = SaveExr( filename, width, height, channels, pixels );
    }

    if ( !saveSuccessful )
    {
        LOG_ERR( "Could not save image to file '%s'", filename.c_str() );
    }

    return true;
}