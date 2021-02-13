#include "asset/image.hpp"
#include "core/assert.hpp"
#include "core/math.hpp"
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
        LOG_ERR( "error while saving exr '%s'", err );
    }
    for ( int i = 0; i < image.num_channels; ++i )
    {
        free( images[i] );
    }

    return success;
}


static glm::ivec2 GetNearestPixelCoords( glm::vec2 uv, int width, int height )
{
    uv = glm::clamp( uv, glm::vec2( 0 ), glm::vec2( 1 ) );
    int row = std::min( height - 1, static_cast< int >( height * uv.y ) );
    int col = std::min( width - 1, static_cast< int >( width * uv.x ) );

    return { row, col };
}


static glm::vec2 CartesianDirToEquirectangular( const glm::vec3& dir )
{
    float phi = atan2( dir.x, dir.z ); // -pi to pi
    float theta = asin( dir.y ); // -pi/2 to pi/2
    
    float u = 0.5f*(phi/PI + 1);
    float v = 0.5f - theta/PI;
    return { u, v };
}


namespace PG
{


Image2D::Image2D( int w, int h ) : width( w ), height( h ), pixels( static_cast< glm::vec4* >( malloc( w * h * sizeof( glm::vec4 ) ) ) )
{
}


Image2D::~Image2D()
{
    if ( pixels )
    {
        free( pixels );
    }
}


Image2D::Image2D( Image2D&& src )
{
    *this = std::move( src );
}


Image2D& Image2D::operator=( Image2D&& src )
{
    name   = std::move( src.name );
    width  = src.width;
    height = src.height;
    flags  = src.flags;
    if ( pixels )
    {
        free( pixels );
    }
    pixels        = src.pixels;
    src.pixels    = nullptr;

    return *this;
}


bool Image2D::Load( Image2DCreateInfo* createInfo )
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
            LOG_ERR( "Tinyexr error '%s'", err );
        }
    }
    else
    {
        LOG_ERR( "Image filetype '%s'", ext, "' is not supported" );
        return false;
    }

    if ( !loadSuccessful )
    {
        LOG_ERR( "Failed to load image '%s'", filename.c_str() );
    }
    //PG_PROFILE_END( ImageLoad );

    return loadSuccessful;
}


bool Image2D::Save( const std::string& filename ) const
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
        LOG_ERR( "Saving image as filetype '%s' is not supported", ext.c_str() );
        return false;
    }

    if ( !saveSuccessful )
    {
        LOG_ERR( "Could not save image '%' to file '%'", name.c_str(), filename.c_str() );
    }
    PG_PROFILE_CPU_END( ImageSave );

    return true;
}


glm::vec4 Image2D::GetPixel( int row, int col ) const
{
    return pixels[row * width + col];
}


void Image2D::SetPixel( int row, int col, const glm::vec4 &pixel )
{
    pixels[row * width + col] = pixel;
}


void Image2D::SetPixel( int row, int col, const glm::vec3& pixel )
{
    pixels[row * width + col] = glm::vec4( pixel, 1 );
}


glm::vec4 Image2D::SampleNearest( glm::vec2 uv ) const
{
    glm::ivec2 rowCol = GetNearestPixelCoords( uv, width, height );
    return GetPixel( rowCol.x, rowCol.y );
}


glm::vec4 Image2D::SampleBilinear( glm::vec2 uv ) const
{
    uv = glm::clamp( uv, glm::vec2( 0 ), glm::vec2( 1 ) );
    uv = uv * glm::vec2( width, height );
    glm::ivec2 uv1 = { std::floor( uv.x ), std::floor( uv.y ) };
    glm::ivec2 uv2 = { std::ceil( uv.x ),  std::floor( uv.y ) };
    glm::ivec2 uv3 = { std::floor( uv.x ), std::ceil( uv.y ) };
    glm::ivec2 uv4 = { std::ceil( uv.x ),  std::ceil( uv.y ) };
    glm::ivec2 minPixelCoord = { 0, 0 };
    glm::ivec2 maxPixelCoord = { width - 1, height - 1 };
    auto Auv1 = glm::clamp( uv1, minPixelCoord, maxPixelCoord );
    auto Auv2 = glm::clamp( uv2, minPixelCoord, maxPixelCoord );
    auto Auv3 = glm::clamp( uv3, minPixelCoord, maxPixelCoord );
    auto Auv4 = glm::clamp( uv4, minPixelCoord, maxPixelCoord );

    float diffX = uv.x - Auv1.x;
    glm::vec4 P1 = (1 - diffX) * GetPixel( Auv1.y, Auv1.x ) + diffX * GetPixel( Auv2.y, Auv2.x );
    glm::vec4 P2 = (1 - diffX) * GetPixel( Auv3.y, Auv3.x ) + diffX * GetPixel( Auv4.y, Auv4.x );
    
    float diffY = uv.y - Auv1.y;
    glm::vec4 finalPoint = (1 - diffY) * P1 + diffY * P2;
    return finalPoint;
}


glm::vec4 Image2D::SampleEquirectangularNearest( const glm::vec3& dir ) const
{
    glm::vec2 equiUV = CartesianDirToEquirectangular( dir );
    return SampleNearest( equiUV );
}


glm::vec4 Image2D::SampleEquirectangularBilinear( const glm::vec3& dir ) const
{
    glm::vec2 equiUV = CartesianDirToEquirectangular( dir );
    return SampleBilinear( equiUV );
}


// +x -> right, +z -> forward, +y -> up
// uv (0, 0) is upper left corner of image
glm::vec3 CubemapTexCoordToCartesianDir( int cubeFace, glm::vec2 uv )
{
    uv = 2.0f * uv - glm::vec2( 1.0f );
    uv.y *= -1; // since uv=(0,0) is assumed upper left corner
    glm::vec3 dir;
    switch ( cubeFace )
    {
    case FACE_BACK:
        dir = glm::vec3( -uv.x, uv.y, -1 );
        break;
    case FACE_LEFT:
        dir = glm::vec3( -1, uv.y, uv.x );
        break;
    case FACE_FRONT:
        dir = glm::vec3( uv.x, uv.y, 1 );
        break;
    case FACE_RIGHT:
        dir = glm::vec3( 1, uv.y, -uv.x );
        break;
    case FACE_TOP:
        dir = glm::vec3( uv.x, 1, -uv.y );
        break;
    case FACE_BOTTOM:
        dir = glm::vec3( uv.x, -1, uv.y );
        break;
    }

    return glm::normalize( dir );
}


ImageCubemap::ImageCubemap( int size )
{
    for ( int i = 0; i < 6; ++i )
    {
        faces[i] = Image2D( size, size );
    }
}


ImageCubemap::ImageCubemap( ImageCubemap&& src )
{
    *this = std::move( src );
}


ImageCubemap& ImageCubemap::operator=( ImageCubemap&& src )
{
    name = std::move( src.name );
    for ( int i = 0; i < 6; ++i )
    {
        faces[i] = std::move( src.faces[i] );
    }

    return *this;
}


bool ImageCubemap::Load( ImageCubemapCreateInfo* createInfo )
{
    if ( !createInfo->equirectangularFilename.empty() )
    {
        Image2DCreateInfo info = {};
        info.filename = createInfo->equirectangularFilename;
        Image2D equirectangularImg;
        if ( !equirectangularImg.Load( &info ) )
        {
            return false;
        }

        int width = equirectangularImg.width / 4;
        int height = width;
        for ( int i = 0; i < 6; ++i )
        {
            faces[i] = Image2D( width, height );
            for ( int r = 0; r < height; ++r )
            {
                for ( int c = 0; c < width; ++ c )
                {
                    glm::vec2 localUV = { (c + 0.5f) / (float)width, (r + 0.5f) / (float)height };
                    glm::vec3 dir = CubemapTexCoordToCartesianDir( i, localUV );
                    glm::vec2 equiUV = CartesianDirToEquirectangular( dir );
                    faces[i].SetPixel( r, c, equirectangularImg.SampleBilinear( equiUV ) );
                }
            }
        }
    }
    else if ( !createInfo->flattenedCubemapFilename.empty() )
    {
        return false;
    }
    else if ( !createInfo->faceFilenames[0].empty() )
    {
        return false;
    }

    return true;
}


bool ImageCubemap::SaveAsFlattenedCubemap( const std::string& filename ) const
{
    int width = faces[0].width;
    int height = faces[0].height;
    int flattenedWidth = 4 * width;
    int flattenedHeight = 3 * height;
    Image2D flattenedCubemap( flattenedWidth, flattenedHeight ); // 4x3 squares, unwrapping the cubemap
    for ( int i = 0; i < flattenedWidth * flattenedHeight; ++i )
    {
        flattenedCubemap.pixels[i] = glm::vec4( 0, 0, 0, 1 );
    }
    int indices[12] =
    {
        -1,        -1,        FACE_TOP,     -1,
        FACE_BACK, FACE_LEFT, FACE_FRONT,   FACE_RIGHT,
        -1,        -1,        FACE_BOTTOM,  -1
    };

    for ( int i = 0; i < 12; ++i )
    {
        int cubemapIndex = indices[i];
        if ( cubemapIndex == -1 )
        {
            continue;
        }

        int startRow = height * (i / 4);
        int startCol = width * (i % 4);
        for ( int r = 0; r < height; ++r )
        {
            for ( int c = 0; c < width; ++ c )
            {
                flattenedCubemap.SetPixel( startRow + r, startCol + c, faces[cubemapIndex].GetPixel( r, c ) );
            }
        }
    }

    return flattenedCubemap.Save( filename );
}


bool ImageCubemap::SaveIndividualFaces( const std::string& filename ) const
{
    return false;
}


bool ImageCubemap::SaveAsEquirectangular( const std::string& filename ) const
{
    return false;
}

    
glm::vec4 ImageCubemap::GetPixel( int face, int row, int col ) const
{
    return faces[face].GetPixel( row, col );
}


void ImageCubemap::SetPixel( int face, int row, int col, const glm::vec4 &pixel )
{
    faces[face].SetPixel( row, col, pixel );
}


void ImageCubemap::SetPixel( int face, int row, int col, const glm::vec3 &pixel )
{
    faces[face].SetPixel( row, col, pixel );
}


// assumes v is normalized, and that +x = right, +y = up, +z = forward
glm::vec2 SampleCube( const glm::vec3& v, int& faceIndex )
{
    glm::vec3 vAbs = abs( v );
	float ma;
	glm::vec2 uv;
	if ( vAbs.z >= vAbs.x && vAbs.z >= vAbs.y )
	{
		faceIndex = v.z < 0.0f ? 0 : 2;
		ma = 0.5f / vAbs.z;
		uv = glm::vec2( v.z < 0.0f ? -v.x : v.x, -v.y );
	}
	else if ( vAbs.y >= vAbs.x )
	{
		faceIndex = v.y < 0.0f ? 5 : 4;
		ma = 0.5f / vAbs.y;
		uv = glm::vec2( v.x, v.y < 0.0f ? -v.z : v.z );
	}
	else
	{
		faceIndex = v.x < 0.0 ? 1 : 3;
		ma = 0.5f / vAbs.x;
		uv = glm::vec2( v.x < 0.0f ? v.z : -v.z, -v.y );
	}
	return uv * ma + glm::vec2( 0.5f );
}


glm::vec4 ImageCubemap::SampleNearest( const glm::vec3& direction ) const
{
    int cubeFaceIndex;
    glm::vec2 cubeFaceUV = SampleCube( direction, cubeFaceIndex );
    return faces[cubeFaceIndex].SampleNearest( cubeFaceUV );
}


glm::vec4 ImageCubemap::SampleBilinear( const glm::vec3& direction ) const
{
    int cubeFaceIndex;
    glm::vec2 cubeFaceUV = SampleCube( direction, cubeFaceIndex );
    return faces[cubeFaceIndex].SampleBilinear( cubeFaceUV );
}

} // namespace PG
