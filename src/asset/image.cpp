#include "asset/image.hpp"
#include "core/assert.hpp"
#include "core/pixel_formats.hpp"
#define STBI_NO_PIC
#define STBI_NO_PNM   
#define STBI_NO_PSD   
#define STBI_NO_GIF   
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb/stb_image_resize.h"
#include "tinyexr/tinyexr.h"
#include "utils/logger.hpp"
#include "utils/filesystem.hpp"
#include "utils/cpu_profile.hpp"


static unsigned char UnormFloatToUChar( float x )
{
    int ret = static_cast<int>( 0.5f + 255.0f * x );
    if ( ret < 0 )   ret = 0;
    if ( ret > 255 ) ret = 255;
    return static_cast< unsigned char >( ret );
}


bool SaveExr( const std::string& filename, int width, int height, glm::vec4* pixels )
{
    EXRImage image;
    InitEXRImage( &image );

    image.num_channels = 4;

    float* images[4];
    for ( int i = 0; i < image.num_channels; ++i )
    {
        images[i] = static_cast< float* >( malloc( width * height * sizeof( float ) ) );
    }

    for (int i = 0; i < width * height; ++i )
    {
        images[0][i] = pixels[i].b;
        images[1][i] = pixels[i].g;
        images[2][i] = pixels[i].r;
        images[3][i] = pixels[i].a;
    }

    image.images = reinterpret_cast< unsigned char** >( images );
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
    strncpy( header.channels[3].name, "A", 255 ); header.channels[3].name[strlen( "A" )] = '\0';

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
        LOG_ERR( "error while saving exr '%s'\n", err );
    }
    for ( int i = 0; i < image.num_channels; ++i )
    {
        free( images[i] );
    }

    return success;
}


namespace PG
{


Image::Image( int w, int h ) : width( w ), height( h ), pixels( static_cast< glm::vec4* >( malloc( w * h * sizeof( glm::vec4 ) ) ) )
{
}


Image::~Image()
{
    if ( pixels )
    {
        free( pixels );
    }
}


Image::Image( Image&& src )
{
    *this = std::move( src );
}


Image& Image::operator=( Image&& src )
{
    if ( pixels )
    {
        free( pixels );
    }
    pixels        = src.pixels;
    src.pixels    = nullptr;

    return *this;
}


bool Image::Load( ImageCreateInfo* createInfo )
{
    //PG_PROFILE_START( ImageLoad );
    PG_ASSERT( createInfo );
    PG_ASSERT( createInfo->filename != "" );
    name     = createInfo->name;
    flags    = createInfo->flags;
    const std::string& filename = createInfo->filename;

    bool loadSuccessful = true;
    std::string ext = GetFileExtension( filename );
    if ( ext == ".jpg" || ext == ".png" || ext == ".tga" || ext == ".bmp" || ext == ".hdr" )
    {
        stbi_set_flip_vertically_on_load( flags & IMAGE_FLIP_VERTICALLY );
        stbi_ldr_to_hdr_gamma( 1.0f );
        stbi_ldr_to_hdr_scale( 1.0f );
        int numComponents;
        pixels = reinterpret_cast<glm::vec4*>( stbi_loadf( filename.c_str(), &width, &height, &numComponents, 4 ) );
        loadSuccessful = pixels != nullptr;

        // assume that hdr/exr images are saved in linear space already
        if ( ext != ".hdr" )
        {
            #pragma omp parallel for
            for ( int r = 0; r < height; r++ )
            {
                for ( int c = 0; c < width; ++c )
                {
                    glm::vec4& pixel = pixels[r*width + c];
                    for ( int channel = 0; channel < 3; ++channel )
                    {
                        pixel[channel] = GammaSRGBToLinear( pixel[channel] );
                    }
                }
            }
        }
    }
    else if ( ext == ".exr" )
    {
        const char* err = nullptr;
        loadSuccessful = LoadEXR( reinterpret_cast<float**>( &pixels ), &width, &height, filename.c_str(), &err ) == TINYEXR_SUCCESS;
        if( err )
        {
            LOG_ERR( "Tinyexr error '%s'\n", err );
        }
    }
    else
    {
        LOG_ERR( "Image filetype '%s'", ext, "' is not supported\n" );
        return false;
    }

    if ( !loadSuccessful )
    {
        LOG_ERR( "Failed to load image '%s'\n", filename.c_str() );
    }
    //PG_PROFILE_END( ImageLoad );

    return loadSuccessful;
}


bool Image::Save( const std::string& filename ) const
{
    PG_ASSERT( pixels );
    PG_ASSERT( width != 0 && height != 0 );
    PG_PROFILE_CPU_START( ImageSave );
    bool saveSuccessful = true;
    std::string ext = GetFileExtension( filename );
    int numChannels = 4;
    if ( ext == ".jpg" || ext == ".png" || ext == ".tga" || ext == ".bmp" )
    {
        glm::u8vec4* ldrImage = static_cast<glm::u8vec4*>( malloc( width * height * sizeof( glm::u8vec4 ) ) );
        #pragma omp parallel for
        for ( int r = 0; r < height; r++ )
        {
            for ( int c = 0; c < width; ++c )
            {
                glm::vec4 pixel = GetPixel( r, c );
                glm::u8vec4 ldr;
                for ( int channel = 0; channel < 3; ++channel )
                {
                    ldr[channel] = UnormFloatToUChar( LinearToGammaSRGB( pixel[channel] ) );
                }
                ldr[3] = UnormFloatToUChar( pixel.a );
                ldrImage[r*width + c] = ldr;
            }
        }
        int ret = 1;
        switch ( ext[1] )
        {
            case 'p':
                ret = stbi_write_png( filename.c_str(), width, height, numChannels, ldrImage, width * numChannels );
                break;
            case 'j':
                ret = stbi_write_jpg( filename.c_str(), width, height, numChannels, ldrImage, 95 );
                break;
            case 'b':
                ret = stbi_write_bmp( filename.c_str(), width, height, numChannels, ldrImage );
                break;
            case 't':
                ret = stbi_write_tga( filename.c_str(), width, height, numChannels, ldrImage );
                break;
            default:
                ret = 0;
        }
        saveSuccessful = ret != 0;
        free( ldrImage );
    }
    else if ( ext == ".hdr" )
    {
        saveSuccessful = 0 != stbi_write_hdr( filename.c_str(), width, height, numChannels, reinterpret_cast<float*>( pixels ) );
    }
    else if ( ext == ".exr" )
    {
        saveSuccessful = SaveExr( filename, width, height, pixels );
    }
    else
    {
        LOG_ERR( "Saving image as filetype '%s' is not supported\n", ext.c_str() );
        return false;
    }

    if ( !saveSuccessful )
    {
        LOG_ERR( "Could not save image '%' to file '%'\n", name.c_str(), filename.c_str() );
    }
    PG_PROFILE_CPU_END( ImageSave );

    return true;
}


glm::vec4 Image::GetPixel( int row, int col ) const
{
    return pixels[row * width + col];
}


void Image::SetPixel( int row, int col, const glm::vec4 &pixel )
{
    pixels[row * width + col] = pixel;
}


} // namespace PG
