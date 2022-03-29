#pragma once

#define GLM_FORCE_SWIZZLE
#include "glm/glm.hpp"
#include "shared/float_conversions.hpp"
#include <string>
#include <functional>

enum class Sampler
{
    NEAREST,
    BILINEAR,
};

struct Image2DCreateInfo
{
    Image2DCreateInfo() = default;
    Image2DCreateInfo( const char* fname ) : filename( fname ) {}
    Image2DCreateInfo( const std::string& fname ) : filename( fname ) {}

    std::string filename;
    bool flipVertically = false;
};

template< typename T >
class Image2D
{
public:
    using Pixel = glm::vec<4, T>;
    Image2D();
    Image2D( int width, int height, void* rgba = nullptr );
    ~Image2D();
    Image2D( Image2D&& src );
    Image2D& operator=( Image2D&& src );

    Image2D Clone() const;
    Image2D GetBlockPaddedCopy() const;

    bool Load( const Image2DCreateInfo& createInfo );
    bool Save( const std::string& filename ) const;
    void Free();
    Image2D Resize( int newWidth, int newHeight ) const;

    Pixel GetPixel( int r, int c ) const;
    Pixel GetPixelClamped( int r, int c ) const;
    glm::vec4 GetPixelFloat( int r, int c ) const;
    void GetBlock( int blockX, int blockY, Pixel* outBlock ) const;
    void GetBlockClamped( int blockX, int blockY, Pixel* outBlock ) const;
    void SetPixel( int r, int c, const Pixel& p );
    T* Raw() const { return &pixels[0][0]; }
    void Swizzle( int r, int g, int b, int a );
    operator bool() const { return pixels != nullptr; }

    Pixel Sample( glm::vec2 uv, Sampler sampler = Sampler::BILINEAR ) const;
    Pixel SampleEquirectangular( const glm::vec3& dir, Sampler sampler = Sampler::BILINEAR ) const;

    template <typename Func>
    void ForEachPixel( Func F )
    {
        for ( int i = 0; i < width * height; ++i ) F( pixels[i] );
    }

    std::string filename;
    int width;
    int height;
    Pixel* pixels; // always 4 channels

private:
    bool allocated = true;
};

using ImageU8 = Image2D<uint8_t>;
using ImageF16 = Image2D<float16>;
using ImageF32 = Image2D<float>;

template <typename ToType, typename FromType>
Image2D<ToType> ConvertImage( const Image2D<FromType>& img )
{
    Image2D<ToType> newImg( img.width, img.height );
    for ( int i = 0; i < img.width * img.height; ++i )
    {
        if constexpr ( std::is_same_v<float, FromType> && std::is_same_v<uint8_t, ToType> )
        {
            newImg.pixels[i] = UNormFloatToByte( img.pixels[i] );
        }
        else if constexpr ( std::is_same_v<float, FromType> && std::is_same_v<uint16_t, ToType> )
        {
            newImg.pixels[i] = Float32ToFloat16( img.pixels[i] );
        }
        else if constexpr ( std::is_same_v<uint16_t, FromType> && std::is_same_v<uint8_t, ToType> )
        {
            newImg.pixels[i] = UNormFloatToByte( Float16ToFloat32( img.pixels[i] ) );
        }
        else if constexpr ( std::is_same_v<uint16_t, FromType> && std::is_same_v<float, ToType> )
        {
            newImg.pixels[i] = Float16ToFloat32( img.pixels[i] );
        }
        else if constexpr ( std::is_same_v<uint8_t, FromType> && std::is_same_v<float, ToType> )
        {
            newImg.pixels[i] = UNormByteToFloat( img.pixels[i] );
        }
        else if constexpr ( std::is_same_v<uint8_t, FromType> && std::is_same_v<uint16_t, ToType> )
        {
            newImg.pixels[i] = Float32ToFloat16( UNormByteToFloat( img.pixels[i] ) );
        }
        else
        {
            static_assert( false, "Unimplemented conversion types for ConvertImage" );
        }
    }

    return newImg;
}

void ComputeMetrics( const ImageU8& groundTruthImg, const ImageU8& reconstructedImg, int numChannelsToCompare, float& MSE, float& PSNR );


struct ImageCubemapCreateInfo
{
    std::string flattenedCubemapFilename;
    std::string equirectangularFilename;
    std::string faceFilenames[6];
    bool flipVertically = false;
};

enum FaceIndex
{
    FACE_BACK   = 0,
    FACE_LEFT   = 1,
    FACE_FRONT  = 2,
    FACE_RIGHT  = 3,
    FACE_TOP    = 4,
    FACE_BOTTOM = 5,
};

class ImageCubemap
{
public:
    ImageCubemap() = default;
    ImageCubemap( int size );
    ~ImageCubemap() = default;
    ImageCubemap( ImageCubemap&& src );
    ImageCubemap& operator=( ImageCubemap&& src );

    bool Load( ImageCubemapCreateInfo* createInfo );
    bool SaveAsFlattenedCubemap( const std::string& filename ) const;
    // Saves 6 images, named filenamePrefix + _[back/left/front/etc]. + fileEnding
    bool SaveIndividualFaces( const std::string& filenamePrefix, const std::string& fileEnding ) const;
    bool SaveAsEquirectangular( const std::string& filename ) const;
    
    glm::vec4 GetPixel( int face, int row, int col ) const;
    void SetPixel( int face, int row, int col, const glm::vec4 &pixel );
    void SetPixel( int face, int row, int col, const glm::vec3 &pixel );
    glm::vec4 Sample( const glm::vec3& direction, Sampler sampler = Sampler::BILINEAR ) const;

    std::string name;
    ImageF32 faces[6]; // all faces should have same width and height
};

glm::vec3 CubemapTexCoordToCartesianDir( int cubeFace, glm::vec2 uv );

ImageCubemap GenerateIrradianceMap( const ImageCubemap& cubemap, int faceSize = 64 );