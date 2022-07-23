#pragma once

#include "shared/core_defines.hpp"
#define GLM_FORCE_SWIZZLE
#include "glm/glm.hpp"
#include <memory>
#include <string>

enum class ImageSaveFlags : uint32_t
{
    DEFAULT               = 0,
    KEEP_FLOATS_AS_32_BIT = (1u << 0), // will convert f32 to fp16 by default if applicable, like when saving EXRs
};
PG_DEFINE_ENUM_OPS( ImageSaveFlags );

struct RawImage2D
{
    enum class Format : uint8_t
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

    RawImage2D() = default;
    RawImage2D( uint32_t inWidth, uint32_t inHeight, Format inFormat ) : width( inWidth ), height( inHeight ), format( inFormat )
    {
        size_t totalBytes = TotalBytes();
        data = std::make_shared<uint8_t[]>( totalBytes );
    }

    bool Load( const std::string& filename );

    // If the file format doesn't support saving the current Format, then the pixels will be converted to an appropriate format for that file
    bool Save( const std::string& filename, ImageSaveFlags saveFlags = ImageSaveFlags::DEFAULT ) const;

    // Returns a new image with same size + pixel data as the current image, but in the desired Format,
    // as long as the current format and dstFormat are both uncompressed
    RawImage2D Convert( Format dstFormat ) const;

    float GetPixelAsFloat( int row, int col, int chan ) const;
    glm::vec4 GetPixelAsFloat4( int row, int col ) const;
    void SetPixelFromFloat( int row, int col, int chan, float x );
    void SetPixelFromFloat4( int row, int col, glm::vec4 pixel );

    size_t TotalBytes() const
    {
        size_t size = IsBCCompressed() ? PaddedWidth() * PaddedHeight() : width * height;
        return size * BitsPerPixel() / 8;
    }

    uint32_t PaddedWidth() const { return (width + 3) & ~3u; }
    uint32_t PaddedHeight() const { return (height + 3) & ~3u; }
    uint32_t BlocksX() const { return (width + 3) / 4; }
    uint32_t BlocksY() const { return (height + 3) / 4; }
    uint32_t BytesPerBlock() const { return BitsPerPixel() / 2; }

    bool IsBCCompressed() const
    {
        auto idx = static_cast<uint8_t>( format );
        return static_cast<uint8_t>( Format::BC1_UNORM ) <= idx && idx <= static_cast<uint8_t>( Format::BC7_UNORM );
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

    uint8_t* GetCompressedBlock( int blockX, int blockY );
    void GetBlockClamped8Bit( int blockX, int blockY, uint8_t* output ) const;

    uint32_t BitsPerPixel() const
    {
        uint8_t mapping[] =
        {
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

        static_assert( ARRAY_COUNT( mapping ) == static_cast<int>( Format::COUNT ) );
        return mapping[static_cast<uint8_t>( format )];
    }

    uint32_t NumChannels() const
    {
        uint8_t mapping[] =
        {
            0,   // INVALID,
            1,   // R8_UNORM,
            2,   // R8_G8_UNORM,
            3,   // R8_G8_B8_UNORM,
            4,   // R8_G8_B8_A8_UNORM,
            1,   // R16_UNORM,
            2,   // R16_G16_UNORM,
            3,   // R16_G16_B16_UNORM,
            4,   // R16_G16_B16_A16_UNORM,
            1,   // R16_FLOAT,
            2,   // R16_G16_FLOAT,
            3,   // R16_G16_B16_FLOAT,
            4,   // R16_G16_B16_A16_FLOAT,
            1,   // R32_FLOAT,
            2,   // R32_G32_FLOAT,
            3,   // R32_G32_B32_FLOAT,
            4,   // R32_G32_B32_A32_FLOAT,
            3,   // BC1_UNORM,
            4,   // BC2_UNORM,
            4,   // BC3_UNORM,
            1,   // BC4_UNORM,
            1,   // BC4_SNORM,
            2,   // BC5_UNORM,
            2,   // BC5_SNORM,
            3,   // BC6H_U16F,
            3,   // BC6H_S16F,
            4,   // BC7_UNORM,
        };

        static_assert( ARRAY_COUNT( mapping ) == static_cast<int>( Format::COUNT ) );
        return mapping[static_cast<uint8_t>( format )];
    }

    uint32_t width = 0;
    uint32_t height = 0;
    Format format = Format::INVALID;
    std::shared_ptr<uint8_t[]> data;
};


struct FloatImage
{
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t numChannels = 0;
    std::shared_ptr<float[]> data;

    FloatImage() = default;
    FloatImage( uint32_t inWidth, uint32_t inHeight, uint32_t inNumChannels ) : width( inWidth ), height( inHeight ), numChannels( inNumChannels )
    {
        data = std::make_shared<float[]>( width * height * numChannels );
    }

    FloatImage Resize( uint32_t newWidth, uint32_t newHeight ) const;
};