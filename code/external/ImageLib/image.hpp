#pragma once

#include "shared/core_defines.hpp"
#include "shared/float_conversions.hpp"
#include "shared/math_vec.hpp"
#include <memory>
#include <string>
#include <vector>

enum class ImageLoadFlags : uint32_t
{
    DEFAULT         = 0,
    FLIP_VERTICALLY = ( 1u << 0 ),
};
PG_DEFINE_ENUM_OPS( ImageLoadFlags );

enum class ImageSaveFlags : uint32_t
{
    DEFAULT               = 0,
    KEEP_FLOATS_AS_32_BIT = ( 1u << 0 ), // will convert f32 to fp16 by default if applicable, like when saving EXRs
};
PG_DEFINE_ENUM_OPS( ImageSaveFlags );

enum class ImageFormat : uint8_t
{
    INVALID,

    R8_UNORM,
    R8_G8_UNORM,
    R8_G8_B8_UNORM,
    R8_G8_B8_A8_UNORM,

    R16_UNORM,
    R16_G16_UNORM,
    R16_G16_B16_UNORM,
    R16_G16_B16_A16_UNORM,

    R16_FLOAT,
    R16_G16_FLOAT,
    R16_G16_B16_FLOAT,
    R16_G16_B16_A16_FLOAT,

    R32_FLOAT,
    R32_G32_FLOAT,
    R32_G32_B32_FLOAT,
    R32_G32_B32_A32_FLOAT,

    BC1_UNORM,
    BC2_UNORM,
    BC3_UNORM,
    BC4_UNORM,
    BC4_SNORM,
    BC5_UNORM,
    BC5_SNORM,
    BC6H_U16F,
    BC6H_S16F,
    BC7_UNORM,

    COUNT
};

constexpr bool IsFormat8BitUnorm( ImageFormat format )
{
    return Underlying( ImageFormat::R8_UNORM ) <= Underlying( format ) &&
           Underlying( format ) <= Underlying( ImageFormat::R8_G8_B8_A8_UNORM );
}

constexpr bool IsFormat16BitUnorm( ImageFormat format )
{
    return Underlying( ImageFormat::R16_UNORM ) <= Underlying( format ) &&
           Underlying( format ) <= Underlying( ImageFormat::R16_G16_B16_A16_UNORM );
}

constexpr bool IsFormat16BitFloat( ImageFormat format )
{
    return Underlying( ImageFormat::R16_FLOAT ) <= Underlying( format ) &&
           Underlying( format ) <= Underlying( ImageFormat::R16_G16_B16_A16_FLOAT );
}

constexpr bool IsFormat32BitFloat( ImageFormat format )
{
    return Underlying( ImageFormat::R32_FLOAT ) <= Underlying( format ) &&
           Underlying( format ) <= Underlying( ImageFormat::R32_G32_B32_A32_FLOAT );
}

constexpr bool IsFormatBCCompressed( ImageFormat format )
{
    return Underlying( ImageFormat::BC1_UNORM ) <= Underlying( format ) && Underlying( format ) <= Underlying( ImageFormat::BC7_UNORM );
}

constexpr ImageFormat GetFormatAfterDecompression( ImageFormat format )
{
    switch ( format )
    {
    case ImageFormat::BC1_UNORM:
    case ImageFormat::BC2_UNORM:
    case ImageFormat::BC3_UNORM:
    case ImageFormat::BC7_UNORM: return ImageFormat::R8_G8_B8_A8_UNORM;
    case ImageFormat::BC4_UNORM:
    case ImageFormat::BC4_SNORM: return ImageFormat::R8_UNORM;
    case ImageFormat::BC5_UNORM:
    case ImageFormat::BC5_SNORM: return ImageFormat::R8_G8_UNORM;
    case ImageFormat::BC6H_U16F:
    case ImageFormat::BC6H_S16F: return ImageFormat::R16_G16_B16_FLOAT;
    default: return ImageFormat::INVALID;
    }
}

inline uint32_t BitsPerPixel( ImageFormat format )
{
    static uint8_t mapping[] = {
        0,   // INVALID,
        8,   // R8_UNORM,
        16,  // R8_G8_UNORM,
        24,  // R8_G8_B8_UNORM,
        32,  // R8_G8_B8_A8_UNORM,
        16,  // R16_UNORM,
        32,  // R16_G16_UNORM,
        48,  // R16_G16_B16_UNORM,
        64,  // R16_G16_B16_A16_UNORM,
        16,  // R16_FLOAT,
        32,  // R16_G16_FLOAT,
        48,  // R16_G16_B16_FLOAT,
        64,  // R16_G16_B16_A16_FLOAT,
        32,  // R32_FLOAT,
        64,  // R32_G32_FLOAT,
        96,  // R32_G32_B32_FLOAT,
        128, // R32_G32_B32_A32_FLOAT,
        4,   // BC1_UNORM,
        8,   // BC2_UNORM,
        8,   // BC3_UNORM,
        4,   // BC4_UNORM,
        4,   // BC4_SNORM,
        8,   // BC5_UNORM,
        8,   // BC5_SNORM,
        8,   // BC6H_U16F,
        8,   // BC6H_S16F,
        8,   // BC7_UNORM,
    };

    static_assert( ARRAY_COUNT( mapping ) == static_cast<int>( ImageFormat::COUNT ) );
    return mapping[Underlying( format )];
}

inline uint32_t NumChannels( ImageFormat format )
{
    static constexpr uint8_t mapping[] = {
        0, // INVALID,
        1, // R8_UNORM,
        2, // R8_G8_UNORM,
        3, // R8_G8_B8_UNORM,
        4, // R8_G8_B8_A8_UNORM,
        1, // R16_UNORM,
        2, // R16_G16_UNORM,
        3, // R16_G16_B16_UNORM,
        4, // R16_G16_B16_A16_UNORM,
        1, // R16_FLOAT,
        2, // R16_G16_FLOAT,
        3, // R16_G16_B16_FLOAT,
        4, // R16_G16_B16_A16_FLOAT,
        1, // R32_FLOAT,
        2, // R32_G32_FLOAT,
        3, // R32_G32_B32_FLOAT,
        4, // R32_G32_B32_A32_FLOAT,
        3, // BC1_UNORM,
        4, // BC2_UNORM,
        4, // BC3_UNORM,
        1, // BC4_UNORM,
        1, // BC4_SNORM,
        2, // BC5_UNORM,
        2, // BC5_SNORM,
        3, // BC6H_U16F,
        3, // BC6H_S16F,
        4, // BC7_UNORM,
    };

    static_assert( ARRAY_COUNT( mapping ) == static_cast<int>( ImageFormat::COUNT ) );
    return mapping[Underlying( format )];
}

inline bool IsImageFilenameBuiltin( const std::string& filename ) { return !filename.empty() && filename[0] == '$'; }

enum class Channel : uint8_t
{
    R = 0,
    G = 1,
    B = 2,
    A = 3,

    COUNT = 4
};

struct RawImage2D
{
    uint32_t width     = 0;
    uint32_t height    = 0;
    ImageFormat format = ImageFormat::INVALID;
    std::shared_ptr<uint8_t[]> data;

    RawImage2D() = default;
    RawImage2D( uint32_t inWidth, uint32_t inHeight, ImageFormat inFormat ) : width( inWidth ), height( inHeight ), format( inFormat )
    {
        size_t totalBytes = TotalBytes();
        data              = std::make_shared<uint8_t[]>( totalBytes );
    }
    RawImage2D( uint32_t inWidth, uint32_t inHeight, ImageFormat inFormat, uint8_t* srcData )
        : width( inWidth ), height( inHeight ), format( inFormat )
    {
        size_t totalBytes = TotalBytes();
        data              = std::shared_ptr<uint8_t[]>( srcData, []( uint8_t* x ) {} );
    }

    bool Load( const std::string& filename, ImageLoadFlags loadFlags = ImageLoadFlags::DEFAULT );

    // If the file format doesn't support saving the current Format, then the pixels will be converted to an appropriate format for that
    // file
    bool Save( const std::string& filename, ImageSaveFlags saveFlags = ImageSaveFlags::DEFAULT ) const;

    // Returns a new image with same size + pixel data as the current image, but in the desired Format,
    // as long as the dstFormat is uncompressed. If the current format is compressed, decompression will happen
    // to the default for
    RawImage2D Convert( ImageFormat dstFormat ) const;

    RawImage2D Clone() const;
    void Clear( vec4 color = vec4( 0, 0, 0, 1 ) );

    uint8_t GetPixelAsByte( int row, int col, int chan ) const;
    u8vec4 GetPixelAsByte4( int row, int col ) const;
    float GetPixelAsFloat( int row, int col, int chan ) const;
    vec4 GetPixelAsFloat4( int row, int col ) const;

    void SetPixelFromFloat( int row, int col, int chan, float x );
    void SetPixelFromFloat4( int row, int col, vec4 pixel );

    uint8_t* GetCompressedBlock( int blockX, int blockY );
    const uint8_t* GetCompressedBlock( int blockX, int blockY ) const;
    uint8_t* GetCompressedBlock( int blockIndex );
    const uint8_t* GetCompressedBlock( int blockIndex ) const;
    void GetBlockClamped8Bit( int blockX, int blockY, uint8_t* outputRGBA ) const;
    void GetBlockClamped16F( int blockX, int blockY, float16* outputRGB ) const;

    uint32_t BitsPerPixel() const { return ::BitsPerPixel( format ); }
    uint32_t NumChannels() const { return ::NumChannels( format ); }
    uint32_t PaddedWidth() const { return ( width + 3 ) & ~3u; }
    uint32_t PaddedHeight() const { return ( height + 3 ) & ~3u; }
    uint32_t BlocksX() const { return ( width + 3 ) / 4; }
    uint32_t BlocksY() const { return ( height + 3 ) / 4; }
    uint32_t TotalBlocks() const { return BlocksX() * BlocksY(); }
    uint32_t BytesPerBlock() const { return BitsPerPixel() * 16 / 8; }

    size_t TotalBytes() const
    {
        size_t numPixels = IsFormatBCCompressed( format ) ? PaddedWidth() * PaddedHeight() : width * height;
        return numPixels * BitsPerPixel() / 8;
    }

    template <typename T = uint8_t>
    T* Raw()
    {
        return reinterpret_cast<T*>( data.get() );
    }

    template <typename T = uint8_t>
    const T* Raw() const
    {
        return reinterpret_cast<const T*>( data.get() );
    }
};

struct FloatImage2D
{
    uint32_t width       = 0;
    uint32_t height      = 0;
    uint32_t numChannels = 0;
    std::shared_ptr<float[]> data;

    FloatImage2D() = default;
    FloatImage2D( uint32_t inWidth, uint32_t inHeight, uint32_t inNumChannels )
        : width( inWidth ), height( inHeight ), numChannels( inNumChannels )
    {
        data = std::make_shared<float[]>( width * height * numChannels );
    }

    // Currently just calls RawImage2D::Load, and then FloatImageFromRawImage2D
    bool Load( const std::string& filename, ImageLoadFlags loadFlags = ImageLoadFlags::DEFAULT );

    // Currently just calls RawImage2DFromFloatImage, and then RawImage2D::Save
    bool Save( const std::string& filename, ImageSaveFlags saveFlags = ImageSaveFlags::DEFAULT ) const;

    FloatImage2D Resize( uint32_t newWidth, uint32_t newHeight ) const;
    FloatImage2D Clone() const;

    template <typename Func>
    void ForEachPixelIndex( Func F )
    {
        for ( uint32_t i = 0; i < width * height; ++i )
            F( i );
    }

    template <typename Func>
    void ForEachPixel( Func F )
    {
        for ( uint32_t i = 0; i < width * height; ++i )
        {
            F( &data[i * numChannels] );
        }
    }

    vec4 Sample( vec2 uv, bool clampHorizontal, bool clampVertical ) const; // bilinear
    vec4 GetFloat4( uint32_t pixelIndex ) const;
    vec4 GetFloat4( uint32_t row, uint32_t col ) const;
    vec4 GetFloat4( uint32_t row, uint32_t col, bool clampHorizontal, bool clampVertical ) const;
    void SetFromFloat4( uint32_t pixelIndex, const vec4& pixel );
    void SetFromFloat4( uint32_t row, uint32_t col, const vec4& pixel );
};

enum FaceIndex
{
    FACE_BACK   = 0,
    FACE_LEFT   = 1,
    FACE_FRONT  = 2,
    FACE_RIGHT  = 3,
    FACE_TOP    = 4,
    FACE_BOTTOM = 5,

    NUM_FACES = 6
};

// Note: NOT the opengl coordinate space. I assume right handed coordinates, with +x -> right, +y -> forward, +z -> up
// and the top left corner of each face is uv (0,0).
// The bottom & top faces have orientation following the 'front' face. Like here:
// https://en.wikipedia.org/wiki/Cube_mapping#Skylight_illumination, except that they use +Y up and bottom left (0,0) uv
struct FloatImageCubemap
{
    uint32_t size        = 0;
    uint32_t numChannels = 0;
    FloatImage2D faces[6];

    FloatImageCubemap() = default;
    FloatImageCubemap( uint32_t inSize, uint32_t inNumChannels ) : size( inSize ), numChannels( inNumChannels )
    {
        for ( int i = 0; i < 6; ++i )
            faces[i] = FloatImage2D( size, size, inNumChannels );
    }

    bool Load( const std::string filenames[6] );
    bool Load( RawImage2D* rawImgs );

    // just for debug visualization + confirmation
    bool SaveUnfoldedFaces( const std::string& filename ) const;
    FloatImageCubemap Resize( uint32_t newSize ) const;

    vec4 Sample( const vec3& dir ) const;

private:
    vec4 FetchTexel_Wrapping( int faceIdx, int row, int col ) const;
};

vec3 CubemapFaceUVToDirection( int cubeFace, vec2 uv );
vec2 CubemapDirectionToFaceUV( const vec3& direction, int& faceIndex );
vec2 DirectionToEquirectangularUV( const vec3& dir );
vec3 EquirectangularUVToDirection( const vec2& uv );

// Creates a new image in the float32 version of rawImage. One caveat: if the raw format is already float32,
// then data will just point to the same RawImage2D memory to avoid an allocation + copy
FloatImage2D FloatImageFromRawImage2D( const RawImage2D& rawImage );

// Creates a new raw image with the specified format. Like FloatImageFromRawImage2D, if "format" is already the same as the float image,
// then the returned raw image isn't "new", it just points to the same memory as the float image to avoid an allocation + copy
// If format == ImageFormat::INVALID, then it just uses the float image's original format
RawImage2D RawImage2DFromFloatImage( const FloatImage2D& floatImage, ImageFormat format = ImageFormat::INVALID );
std::vector<RawImage2D> RawImage2DFromFloatImages(
    const std::vector<FloatImage2D>& floatImages, ImageFormat format = ImageFormat::INVALID );

FloatImageCubemap EquirectangularToCubemap( const FloatImage2D& equiImg );
FloatImage2D CubemapToEquirectangular( const FloatImageCubemap& cubemap );

struct MipmapGenerationSettings
{
    bool clampHorizontal = false;
    bool clampVertical   = false;
};

std::vector<FloatImage2D> GenerateMipmaps( const FloatImage2D& floatImage, const MipmapGenerationSettings& settings );

uint32_t CalculateNumMips( uint32_t width, uint32_t height );
double FloatImageMSE( const FloatImage2D& img1, const FloatImage2D& img2, uint32_t channelsToCalc = 0b1111 );
double MSEToPSNR( double mse, double maxValue = 1.0 );
