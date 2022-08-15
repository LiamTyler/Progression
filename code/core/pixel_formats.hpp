#pragma once

#include <string>

namespace PG
{


enum class PixelFormat : uint8_t
{
    INVALID                 = 0,

    R8_UNORM                = 1,
    R8_SNORM                = 2,
    R8_UINT                 = 3,
    R8_SINT                 = 4,
    R8_SRGB                 = 5,

    R8_G8_UNORM             = 6,
    R8_G8_SNORM             = 7,
    R8_G8_UINT              = 8,
    R8_G8_SINT              = 9,
    R8_G8_SRGB              = 10,

    R8_G8_B8_UNORM          = 11,
    R8_G8_B8_SNORM          = 12,
    R8_G8_B8_UINT           = 13,
    R8_G8_B8_SINT           = 14,
    R8_G8_B8_SRGB           = 15,

    B8_G8_R8_UNORM          = 16,
    B8_G8_R8_SNORM          = 17,
    B8_G8_R8_UINT           = 18,
    B8_G8_R8_SINT           = 19,
    B8_G8_R8_SRGB           = 20,

    R8_G8_B8_A8_UNORM       = 21,
    R8_G8_B8_A8_SNORM       = 22,
    R8_G8_B8_A8_UINT        = 23,
    R8_G8_B8_A8_SINT        = 24,
    R8_G8_B8_A8_SRGB        = 25,

    B8_G8_R8_A8_UNORM       = 26,
    B8_G8_R8_A8_SNORM       = 27,
    B8_G8_R8_A8_UINT        = 28,
    B8_G8_R8_A8_SINT        = 29,
    B8_G8_R8_A8_SRGB        = 30,

    R16_UNORM               = 31,
    R16_SNORM               = 32,
    R16_UINT                = 33,
    R16_SINT                = 34,
    R16_FLOAT               = 35,

    R16_G16_UNORM           = 36,
    R16_G16_SNORM           = 37,
    R16_G16_UINT            = 38,
    R16_G16_SINT            = 39,
    R16_G16_FLOAT           = 40,

    R16_G16_B16_UNORM       = 41,
    R16_G16_B16_SNORM       = 42,
    R16_G16_B16_UINT        = 43,
    R16_G16_B16_SINT        = 44,
    R16_G16_B16_FLOAT       = 45,

    R16_G16_B16_A16_UNORM   = 46,
    R16_G16_B16_A16_SNORM   = 47,
    R16_G16_B16_A16_UINT    = 48,
    R16_G16_B16_A16_SINT    = 49,
    R16_G16_B16_A16_FLOAT   = 50,

    R32_UINT                = 51,
    R32_SINT                = 52,
    R32_FLOAT               = 53,

    R32_G32_UINT            = 54,
    R32_G32_SINT            = 55,
    R32_G32_FLOAT           = 56,

    R32_G32_B32_UINT        = 57,
    R32_G32_B32_SINT        = 58,
    R32_G32_B32_FLOAT       = 59,

    R32_G32_B32_A32_UINT    = 60,
    R32_G32_B32_A32_SINT    = 61,
    R32_G32_B32_A32_FLOAT   = 62,

    DEPTH_16_UNORM                  = 63,
    DEPTH_32_FLOAT                  = 64,
    DEPTH_16_UNORM_STENCIL_8_UINT   = 65,
    DEPTH_24_UNORM_STENCIL_8_UINT   = 66,
    DEPTH_32_FLOAT_STENCIL_8_UINT   = 67,

    STENCIL_8_UINT          = 68,

    BC1_RGB_UNORM           = 69,
    BC1_RGB_SRGB            = 70,
    BC1_RGBA_UNORM          = 71,
    BC1_RGBA_SRGB           = 72,
    BC2_UNORM               = 73,
    BC2_SRGB                = 74,
    BC3_UNORM               = 75,
    BC3_SRGB                = 76,
    BC4_UNORM               = 77,
    BC4_SNORM               = 78,
    BC5_UNORM               = 79,
    BC5_SNORM               = 80,
    BC6H_UFLOAT             = 81,
    BC6H_SFLOAT             = 82,
    BC7_UNORM               = 83,
    BC7_SRGB                = 84,

    NUM_PIXEL_FORMATS
};

uint32_t NumChannelsInPixelFromat( PixelFormat format );

uint32_t NumBytesPerPixel( PixelFormat format );

uint32_t NumBytesPerChannel( PixelFormat format );

bool PixelFormatIsNormalized( PixelFormat format );

bool PixelFormatIsUnsigned( PixelFormat format );

bool PixelFormatIsSrgb( PixelFormat format );

bool PixelFormatIsFloat( PixelFormat format );

bool PixelFormatHasDepthFormat( PixelFormat format );

bool PixelFormatHasStencil( PixelFormat format );

bool PixelFormatIsCompressed( PixelFormat format );

// Normally r = 0, g = 1, b = 2, a = 3, besides for a few formats like B8_G8_R8_UNORM
void GetRGBAOrder( PixelFormat format, int channelRemap[4] );

std::string PixelFormatName( PixelFormat format );

PixelFormat PixelFormatFromString( const std::string& format );

} // namespace PG
