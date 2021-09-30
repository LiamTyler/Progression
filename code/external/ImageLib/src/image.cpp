#include "image.hpp"
#include "image_load.hpp"
#include "image_save.hpp"
#include "shared/assert.hpp"
#include "shared/filesystem.hpp"
#include "shared/float_conversions.hpp"
#include "shared/logger.hpp"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb/stb_image_resize.h"
#include <climits>
#include <memory>
#include <vector>

#define PI 3.14159265358979323846f

template <typename T>
glm::vec4 PixelToFloat( glm::vec<4, T> p )
{
    if constexpr ( std::is_same_v<T, uint8_t> )
    {
        return UNormByteToFloat( p );
    }
    else if constexpr ( std::is_same_v<T, float16> )
    {
        return Float16ToFloat32( p );
    }
    else
    {
        return p;
    }
}


template <typename T>
glm::vec<4, T> PixelFromFloat( glm::vec4 p )
{
    if constexpr ( std::is_same_v<T, uint8_t> )
    {
        return UNormFloatToByte( p );
    }
    else if constexpr ( std::is_same_v<T, float16> )
    {
        return Float32ToFloat16( p );
    }
    else
    {
        return p;
    }
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


static glm::vec3 EquirectangularToCartesianDir( const glm::vec2& uv )
{
    float phi = (uv.x * 2.0f - 1) * PI;
    float theta = (0.5f - uv.y) * PI;

    glm::vec3 dir = { cos( theta ) * sin( phi ), sin( theta ), cos( theta ) * cos( phi ) };
    return dir;
}


template< typename T>
Image2D<T>::Image2D() : width( 0 ), height( 0 ), pixels( nullptr )
{
}


template< typename T>
Image2D<T>::Image2D( int w, int h, void* rgba ) : width( w ), height( h )
{
    PG_ASSERT( w >= 1 && w <= 1024 * 64 );
    PG_ASSERT( h >= 1 && h <= 1024 * 64 );
    if ( rgba )
    {
        pixels = reinterpret_cast<Pixel*>( rgba );
        allocated = false;
    }
    else
    {
        pixels = static_cast<Pixel*>( malloc( w * h * sizeof( Pixel ) ) );
    }
}


template< typename T>
Image2D<T>::~Image2D()
{
    if ( pixels && allocated )
    {
        free( pixels );
    }
}


template< typename T>
Image2D<T>::Image2D( Image2D<T>&& src )
{
    pixels = nullptr;
    *this = std::move( src );
}


template< typename T>
Image2D<T>& Image2D<T>::operator=( Image2D<T>&& src )
{
    if ( pixels && allocated )
    {
        free( pixels );
    }

    filename   = std::move( src.filename );
    pixels     = src.pixels;
    src.pixels = nullptr;
    width      = src.width;
    height     = src.height;

    return *this;
}


template< typename T>
Image2D<T> Image2D<T>::Clone() const
{
    Image2D img;
    img.width = width;
    img.height = height;
    img.filename = filename;
    img.pixels = nullptr;
    if ( pixels )
    {
        img.pixels = static_cast<Pixel*>( malloc( width * height * sizeof( Pixel ) ) );
        memcpy( img.pixels, pixels, width * height * sizeof( Pixel ) );
    }

    return img;
}


template< typename T>
Image2D<T> Image2D<T>::GetBlockPaddedCopy() const
{
    int newHeight = (height + 3) / 4;
    int newWidth = (width + 3) / 4;

    Image2D newImg( newWidth, newHeight );
    for ( int y = 0; y < height; ++y )
        for ( int x = 0; x < width; ++x )
            newImg.SetPixel( y, x, GetPixel( y, std::min( width - 1, x ) ) );

	for ( int y = height; y < newHeight; y++ )
		memcpy( &newImg.pixels[newWidth*y][0], &newImg.pixels[newWidth*(y-1)][0], 4 * newWidth );

    return newImg;
}


template< typename T>
bool Image2D<T>::Load( const Image2DCreateInfo& createInfo )
{
    filename = createInfo.filename;
    if ( pixels )
    {
        if ( allocated )
        {
            free( pixels );
        }
        pixels = nullptr;
    }

    if constexpr ( std::is_same_v<float, T> )
    {
        pixels = Load2D_F32( filename, width, height );
    }
    else if constexpr ( std::is_same_v<float16, T> )
    {
        pixels = Load2D_F16( filename, width, height );
    }
    else
    {
        pixels = Load2D_U8( filename, width, height );
    }

    if ( !pixels )
    {
        LOG_ERR( "Failed to load image '%s'", filename.c_str() );
        return false;
    }
    if ( createInfo.flipVertically )
    {
        for ( int r = 0; r < height / 2; ++r )
        {
            for ( int c = 0; c < width; ++c )
            {
                std::swap( pixels[r * width + c], pixels[(height - r - 1) * width + c] );
            }
        }
    }

    return true;
}


template< typename T>
bool Image2D<T>::Save( const std::string& saveFilename ) const
{
    bool ret = false;
    if constexpr ( std::is_same_v<float, T> )
    {
        ret = Save2D_F32( saveFilename, reinterpret_cast<float*>( pixels ), width, height, 4 );
    }
    else if constexpr ( std::is_same_v<float16, T> )
    {
        ret = Save2D_F16( saveFilename, reinterpret_cast<uint16_t*>( pixels ), width, height, 4 );
    }
    else
    {
        ret = Save2D_U8( saveFilename, reinterpret_cast<uint8_t*>( pixels ), width, height, 4 );
    }

    return ret;
}


template< typename T>
typename Image2D<T>::Pixel Image2D<T>::GetPixel( int r, int c ) const
{
    return pixels[width * r + c];
}


template< typename T>
typename Image2D<T>::Pixel Image2D<T>::GetPixelClamped( int r, int c ) const
{
    r = std::max( 0, std::min( height - 1, r ) );
    c = std::max( 0, std::min( width - 1, c ) );
    return GetPixel( r, c );
}


template< typename T>
glm::vec4 Image2D<T>::GetPixelFloat( int r, int c ) const
{
    int y = std::max( 0, std::min( height - 1, r ) );
    int x = std::max( 0, std::min( width - 1, c ) );
    return PixelToFloat( GetPixel( y, x ) );
}



template< typename T>
void Image2D<T>::GetBlock( int blockX, int blockY, Pixel* outBlock ) const
{
    for ( int r = 0; r < 4; ++r )
    {
        for ( int c = 0; c < 4; ++c )
        {
            int row = 4 * blockY + r;
            int col = 4 * blockX + c;
            outBlock[4*r + c] = GetPixel( row, col );
        }
    }
}


template< typename T>
void Image2D<T>::GetBlockClamped( int blockX, int blockY, Pixel* outBlock ) const
{
    for ( int r = 0; r < 4; ++r )
    {
        int row = 4 * blockY + r;
        for ( int c = 0; c < 4; ++c )
        {
            int col = 4 * blockX + c;
            outBlock[4*r + c] = GetPixelClamped( row, col );
        }
    }
}


template< typename T>
void Image2D<T>::SetPixel( int r, int c, const Pixel& p )
{
    pixels[width * r + c] = p;
}


template< typename T>
void Image2D<T>::Swizzle( int r, int g, int b, int a )
{
    PG_ASSERT( r >= 0 && g >= 0 && b >= 0 && a >= 0 );
    PG_ASSERT( r <= 3 && g <= 3 && b <= 3 && a <= 3 );
    for ( int i = 0; i < width * height; ++i )
    {
        auto p = pixels[i];
        pixels[i] = { p[r], p[g], p[b], p[a] };
    }
}


template< typename T>
typename Image2D<T>::Pixel Image2D<T>::Sample( glm::vec2 uv, Sampler sampler ) const
{
    if ( sampler == Sampler::BILINEAR ) [[likely]]
    {
        uv = glm::clamp( uv, glm::vec2( 0 ), glm::vec2( 1 ) );
        uv = uv * glm::vec2( width, height );
        glm::ivec2 uv1 = { std::floor( uv.x ), std::floor( uv.y ) };
        glm::ivec2 uv2 = { std::ceil( uv.x ),  std::floor( uv.y ) };
        glm::ivec2 uv3 = { std::floor( uv.x ), std::ceil( uv.y ) };
        glm::ivec2 uv4 = { std::ceil( uv.x ),  std::ceil( uv.y ) };
        glm::ivec2 minPixelCoord = { 0, 0 };
        glm::ivec2 maxPixelCoord = { width - 1, height - 1 };
        auto Auv1 = glm::min( uv1, maxPixelCoord );
        auto Auv2 = glm::min( uv2, maxPixelCoord );
        auto Auv3 = glm::min( uv3, maxPixelCoord );
        auto Auv4 = glm::min( uv4, maxPixelCoord );

        float diffX = uv.x - Auv1.x;
        glm::vec4 P1 = (1 - diffX) * GetPixelFloat( Auv1.y, Auv1.x ) + diffX * GetPixelFloat( Auv2.y, Auv2.x );
        glm::vec4 P2 = (1 - diffX) * GetPixelFloat( Auv3.y, Auv3.x ) + diffX * GetPixelFloat( Auv4.y, Auv4.x );
    
        float diffY = uv.y - Auv1.y;
        glm::vec4 finalPoint = (1 - diffY) * P1 + diffY * P2;
        return PixelFromFloat<T>( finalPoint );
    }
    else
    {
        glm::ivec2 rowCol = GetNearestPixelCoords( uv, width, height );
        return GetPixel( rowCol.x, rowCol.y );
    }
}


template< typename T>
typename Image2D<T>::Pixel Image2D<T>::SampleEquirectangular( const glm::vec3& dir, Sampler sampler ) const
{
    glm::vec2 equiUV = CartesianDirToEquirectangular( dir );
    return Sample( equiUV, sampler );
}


void ComputeMetrics( const ImageU8& groundTruthImg, const ImageU8& reconstructedImg, int numChannelsToCompare, float& MSE, float& PSNR )
{
    MSE = PSNR = 0;
    if ( groundTruthImg.width != reconstructedImg.width || groundTruthImg.height != reconstructedImg.height )
    {
        LOG_ERR( "Could not compute errors between two images with different dimensions" );
        return;
    }

    glm::dvec4 accum( 0 );
    for ( int i = 0; i < reconstructedImg.width * reconstructedImg.height; ++i )
    {
        glm::vec4 diff = glm::abs( UNormByteToFloat( reconstructedImg.pixels[i] ) - UNormByteToFloat( groundTruthImg.pixels[i] ) );
        accum += diff * diff;
    }

    double total = 0;
    for ( int i = 0; i < numChannelsToCompare; ++i )
    {
        total += accum[i];
    }

    int numSamples = reconstructedImg.height * reconstructedImg.width * numChannelsToCompare;
    MSE = static_cast< float >( total / numSamples );
    PSNR = -10.0f * log10f( MSE );
}

template class Image2D<uint8_t>;
template class Image2D<float16>;
template class Image2D<float>;


static int s_flattenedCubemapFaceLayout[12] =
{
    -1,        -1,        FACE_TOP,     -1,
    FACE_BACK, FACE_LEFT, FACE_FRONT,   FACE_RIGHT,
    -1,        -1,        FACE_BOTTOM,  -1
};


ImageCubemap::ImageCubemap( int size )
{
    for ( int i = 0; i < 6; ++i )
    {
        faces[i] = ImageF32( size, size );
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
        ImageF32 equirectangularImg;
        if ( !equirectangularImg.Load( info ) )
        {
            return false;
        }

        int width = equirectangularImg.width / 4;
        int height = width;
        for ( int i = 0; i < 6; ++i )
        {
            faces[i] = ImageF32( width, height );
            for ( int r = 0; r < height; ++r )
            {
                for ( int c = 0; c < width; ++ c )
                {
                    glm::vec2 localUV = { (c + 0.5f) / (float)width, (r + 0.5f) / (float)height };
                    glm::vec3 dir = CubemapTexCoordToCartesianDir( i, localUV );
                    glm::vec2 equiUV = CartesianDirToEquirectangular( dir );
                    faces[i].SetPixel( r, c, equirectangularImg.Sample( equiUV, Sampler::BILINEAR ) );
                }
            }
        }
    }
    else if ( !createInfo->flattenedCubemapFilename.empty() )
    {
        Image2DCreateInfo img2Dinfo;
        img2Dinfo.filename = createInfo->flattenedCubemapFilename;
        img2Dinfo.flipVertically = createInfo->flipVertically;
        ImageF32 img;
        if ( !img.Load( img2Dinfo ) )
        {
            return false;
        }
        int width = img.width / 4;
        int height = img.height / 3;
        for ( int i = 0; i < 12; ++i )
        {
            int faceIndex = s_flattenedCubemapFaceLayout[i];
            if ( faceIndex == -1 )
            {
                continue;
            }
            faces[faceIndex] = ImageF32( width, height );

            int startRow = height * (i / 4);
            int startCol = width * (i % 4);
            for ( int r = 0; r < height; ++r )
            {
                for ( int c = 0; c < width; ++ c )
                {
                    faces[faceIndex].SetPixel( r, c, img.GetPixel( startRow + r, startCol + c ) );
                }
            }
        }
    }
    else if ( !createInfo->faceFilenames[0].empty() )
    {
        Image2DCreateInfo faceCreateInfo;
        faceCreateInfo.flipVertically = createInfo->flipVertically;
        bool success = true;
        for ( int i = 0; i < 6; ++i )
        {
            faceCreateInfo.filename = createInfo->faceFilenames[i];
            success = success && faces[i].Load( faceCreateInfo );
        }
        return success;
    }
    else
    {
        LOG_ERR( "No input images provided in ImageCubemap::Load" );
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
    ImageF32 flattenedCubemap( flattenedWidth, flattenedHeight ); // 4x3 squares, unwrapping the cubemap
    for ( int i = 0; i < flattenedWidth * flattenedHeight; ++i )
    {
        flattenedCubemap.pixels[i] = glm::vec4( 0, 0, 0, 1 );
    }

    for ( int i = 0; i < 12; ++i )
    {
        int cubemapIndex = s_flattenedCubemapFaceLayout[i];
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


bool ImageCubemap::SaveIndividualFaces( const std::string& filenamePrefix, const std::string& fileEnding ) const
{
    static_assert( FACE_BACK == 0 && FACE_LEFT == 1 && FACE_FRONT == 2 && FACE_RIGHT == 3 && FACE_TOP == 4 );
    std::string faceNames[6] = { "back", "left", "front", "right", "top", "bottom" };
    bool success = true;
    for ( int i = 0; i < 6; ++i )
    {
        std::string filename = filenamePrefix + "_" + faceNames[i] + "." + fileEnding;
        success = success && faces[i].Save( filename );
    }

    return success;
}


bool ImageCubemap::SaveAsEquirectangular( const std::string& filename ) const
{
    int width = faces[0].width * 4;
    int height = faces[0].width * 2;
    ImageF32 eqImg( width, height );
    for ( int r = 0; r < height; ++r )
    {
        for ( int c = 0; c < width; ++c )
        {
            glm::vec2 uv = { (c + 0.5f) / (float)width, (r + 0.5f) / (float)height };
            glm::vec3 dir = EquirectangularToCartesianDir( uv );
            eqImg.SetPixel( r, c, Sample( dir, Sampler::BILINEAR ) );
        }
    }

    return eqImg.Save( filename );
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
    faces[face].SetPixel( row, col, { pixel, 1 } );
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


glm::vec4 ImageCubemap::Sample( const glm::vec3& direction, Sampler sampler ) const
{
    int cubeFaceIndex;
    glm::vec2 cubeFaceUV = SampleCube( direction, cubeFaceIndex );
    return faces[cubeFaceIndex].Sample( cubeFaceUV, sampler );
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


ImageCubemap GenerateIrradianceMap( const ImageCubemap& cubemap, int faceSize )
{
    ImageCubemap irradianceMap( faceSize );
    for ( int faceIndex = 0; faceIndex < 6; ++faceIndex )
    {
        #pragma omp parallel for schedule(dynamic)
        for ( int r = 0; r < faceSize; ++r )
        {
            for ( int c = 0; c < faceSize; ++ c )
            {
                glm::vec2 localUV = { (c + 0.5f) / (float)faceSize, (r + 0.5f) / (float)faceSize };
                glm::vec3 N = CubemapTexCoordToCartesianDir( faceIndex, localUV );
    
                glm::vec3 up = glm::vec3( 0, 1, 0 );
                if ( N == up )
                {
                    up = glm::vec3( -1, 0, 0 );
                }
                glm::vec3 right = glm::cross( N, up );
                up = glm::cross( right, N );
                    
                glm::vec3 irradiance( 0 );
                float numSamples = 0;
                const float STEP_IN_RADIANS = 1.0f / 180.0f * PI;
                for ( float phi = 0; phi < 2 * PI; phi += STEP_IN_RADIANS )
                {
                    for ( float theta = 0; theta < 0.5f * PI; theta += STEP_IN_RADIANS )
                    {
                        glm::vec3 tangentSpaceDir = { sin( theta ) * cos( phi ), sin( theta ) * sin( phi ), cos( theta ) };
                        glm::vec3 worldSpaceDir   = tangentSpaceDir.x * right + tangentSpaceDir.y * up + tangentSpaceDir.z * N;
                        glm::vec3 radiance = cubemap.Sample( worldSpaceDir, Sampler::BILINEAR );
                        irradiance += radiance * cos( theta ) * sin( theta );
                        ++numSamples;
                    }
                }
                irradianceMap.SetPixel( faceIndex, r, c, (PI / numSamples) * irradiance );
            }
        }
    }

    return irradianceMap;
}