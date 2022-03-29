#pragma once

#include <cstdint>

enum class CompressionFormat
{
    INVALID     = 0,        
    BC1_UNORM   = 1,
    BC1_SRGB    = 2,
    BC2_UNORM   = 3,
    BC2_SRGB    = 4,
    BC3_UNORM   = 5,
    BC3_SRGB    = 6,
    BC4_UNORM   = 7,
    BC4_SNORM   = 8,
    BC5_UNORM   = 9,
    BC5_SNORM   = 10,
    BC6H_UFLOAT = 11,
    BC6H_SFLOAT = 12,
    BC7_UNORM   = 13,
    BC7_SRGB    = 14,
};

int NumBytesPerCompressedBlock( CompressionFormat format );
int GetBCNumber( CompressionFormat format );

enum class CompressionQuality
{
    LOWEST,
    MEDIUM,
    HIGHEST,
};

struct BCCompressorSettings
{
    CompressionFormat format = CompressionFormat::INVALID;
    CompressionQuality quality = CompressionQuality::MEDIUM;
    int bc4SourceChannel  = 0;
    int bc5SourceChannel1 = 0;
    int bc5SourceChannel2 = 1;
};

// Expects input data to always be 4 channel RGBA, with 8 bits per channel for BC1,3,4,5 & BC7, and 16bit floats for BC6
bool Compress_RGBASingleMip_To_BC( uint8_t* srcMip, int width, int height, const BCCompressorSettings& settings, uint8_t* compressedOutput );
bool Compress_RGBASingleMip_To_BC_Alloc( uint8_t* srcMip, int width, int height, const BCCompressorSettings& settings, uint8_t** compressedOutput );
bool Compress_RGBAAllMips_To_BC( uint8_t* srcImage, int width, int height, const BCCompressorSettings& settings, uint8_t* compressedOutput );
bool Compress_RGBAAllMips_To_BC_Alloc( uint8_t* srcImage, int width, int height, const BCCompressorSettings& settings, uint8_t** compressedOutput );
