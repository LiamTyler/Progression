#include "pixel_formats.hpp"
#include "shared/assert.hpp"
#include <cmath>
#include <unordered_map>

namespace PG
{

u32 NumChannelsInPixelFromat( PixelFormat format )
{
    u8 components[] = {
        0, // INVALID
        1, // R8_UNORM
        1, // R8_SNORM
        1, // R8_UINT
        1, // R8_SINT
        1, // R8_SRGB
        2, // R8_G8_UNORM
        2, // R8_G8_SNORM
        2, // R8_G8_UINT
        2, // R8_G8_SINT
        2, // R8_G8_SRGB
        3, // R8_G8_B8_UNORM
        3, // R8_G8_B8_SNORM
        3, // R8_G8_B8_UINT
        3, // R8_G8_B8_SINT
        3, // R8_G8_B8_SRGB
        3, // B8_G8_R8_UNORM
        3, // B8_G8_R8_SNORM
        3, // B8_G8_R8_UINT
        3, // B8_G8_R8_SINT
        3, // B8_G8_R8_SRGB
        4, // R8_G8_B8_A8_UNORM
        4, // R8_G8_B8_A8_SNORM
        4, // R8_G8_B8_A8_UINT
        4, // R8_G8_B8_A8_SINT
        4, // R8_G8_B8_A8_SRGB
        4, // B8_G8_R8_A8_UNORM
        4, // B8_G8_R8_A8_SNORM
        4, // B8_G8_R8_A8_UINT
        4, // B8_G8_R8_A8_SINT
        4, // B8_G8_R8_A8_SRGB
        1, // R16_UNORM
        1, // R16_SNORM
        1, // R16_UINT
        1, // R16_SINT
        1, // R16_FLOAT
        2, // R16_G16_UNORM
        2, // R16_G16_SNORM
        2, // R16_G16_UINT
        2, // R16_G16_SINT
        2, // R16_G16_FLOAT
        3, // R16_G16_B16_UNORM
        3, // R16_G16_B16_SNORM
        3, // R16_G16_B16_UINT
        3, // R16_G16_B16_SINT
        3, // R16_G16_B16_FLOAT
        4, // R16_G16_B16_A16_UNORM
        4, // R16_G16_B16_A16_SNORM
        4, // R16_G16_B16_A16_UINT
        4, // R16_G16_B16_A16_SINT
        4, // R16_G16_B16_A16_FLOAT
        1, // R32_UINT
        1, // R32_SINT
        1, // R32_FLOAT
        2, // R32_G32_UINT
        2, // R32_G32_SINT
        2, // R32_G32_FLOAT
        3, // R32_G32_B32_UINT
        3, // R32_G32_B32_SINT
        3, // R32_G32_B32_FLOAT
        4, // R32_G32_B32_A32_UINT
        4, // R32_G32_B32_A32_SINT
        4, // R32_G32_B32_A32_FLOAT
        1, // DEPTH_16_UNORM
        1, // DEPTH_32_FLOAT
        2, // DEPTH_16_UNORM_STENCIL_8_UINT
        2, // DEPTH_24_UNORM_STENCIL_8_UINT
        2, // DEPTH_32_FLOAT_STENCIL_8_UINT
        1, // STENCIL_8_UINT
        4, // BC1_RGB_UNORM    // Still decompressed as 4 channel, user can interpret as 3 tho
        4, // BC1_RGB_SRGB     // Still decompressed as 4 channel, user can interpret as 3 tho
        4, // BC1_RGBA_UNORM
        4, // BC1_RGBA_SRGB
        2, // BC2_UNORM
        4, // BC2_SRGB
        4, // BC3_UNORM
        4, // BC3_SRGB
        1, // BC4_UNORM
        1, // BC4_SNORM
        2, // BC5_UNORM
        2, // BC5_SNORM
        3, // BC6H_UFLOAT
        3, // BC6H_SFLOAT
        3, // BC7_UNORM (can be RGB or RGBA)
        3, // BC7_SRGB  (can be RGB or RGBA)
    };

    PG_ASSERT( Underlying( format ) < Underlying( PixelFormat::NUM_PIXEL_FORMATS ) );
    static_assert( ARRAY_COUNT( components ) == Underlying( PixelFormat::NUM_PIXEL_FORMATS ) );

    return components[static_cast<i32>( format )];
}

u32 NumBytesPerPixel( PixelFormat format )
{
    u8 size[] = {
        0, // INVALID

        1, // R8_UNORM
        1, // R8_SNORM
        1, // R8_UINT
        1, // R8_SINT
        1, // R8_SRGB

        2, // R8_G8_UNORM
        2, // R8_G8_SNORM
        2, // R8_G8_UINT
        2, // R8_G8_SINT
        2, // R8_G8_SRGB

        3, // R8_G8_B8_UNORM
        3, // R8_G8_B8_SNORM
        3, // R8_G8_B8_UINT
        3, // R8_G8_B8_SINT
        3, // R8_G8_B8_SRGB

        3, // B8_G8_R8_UNORM
        3, // B8_G8_R8_SNORM
        3, // B8_G8_R8_UINT
        3, // B8_G8_R8_SINT
        3, // B8_G8_R8_SRGB

        4, // R8_G8_B8_A8_UNORM
        4, // R8_G8_B8_A8_SNORM
        4, // R8_G8_B8_A8_UINT
        4, // R8_G8_B8_A8_SINT
        4, // R8_G8_B8_A8_SRGB

        4, // B8_G8_R8_A8_UNORM
        4, // B8_G8_R8_A8_SNORM
        4, // B8_G8_R8_A8_UINT
        4, // B8_G8_R8_A8_SINT
        4, // B8_G8_R8_A8_SRGB

        2, // R16_UNORM
        2, // R16_SNORM
        2, // R16_UINT
        2, // R16_SINT
        2, // R16_FLOAT

        4, // R16_G16_UNORM
        4, // R16_G16_SNORM
        4, // R16_G16_UINT
        4, // R16_G16_SINT
        4, // R16_G16_FLOAT

        6, // R16_G16_B16_UNORM
        6, // R16_G16_B16_SNORM
        6, // R16_G16_B16_UINT
        6, // R16_G16_B16_SINT
        6, // R16_G16_B16_FLOAT

        8, // R16_G16_B16_A16_UNORM
        8, // R16_G16_B16_A16_SNORM
        8, // R16_G16_B16_A16_UINT
        8, // R16_G16_B16_A16_SINT
        8, // R16_G16_B16_A16_FLOAT

        4, // R32_UINT
        4, // R32_SINT
        4, // R32_FLOAT

        8, // R32_G32_UINT
        8, // R32_G32_SINT
        8, // R32_G32_FLOAT

        12, // R32_G32_B32_UINT
        12, // R32_G32_B32_SINT
        12, // R32_G32_B32_FLOAT

        16, // R32_G32_B32_A32_UINT
        16, // R32_G32_B32_A32_SINT
        16, // R32_G32_B32_A32_FLOAT

        2, // DEPTH_16_UNORM
        4, // DEPTH_32_FLOAT
        3, // DEPTH_16_UNORM_STENCIL_8_UINT
        4, // DEPTH_24_UNORM_STENCIL_8_UINT
        5, // DEPTH_32_FLOAT_STENCIL_8_UINT

        1, // STENCIL_8_UINT

        // Pixel size for the BC formats is the size of the 4x4 block, not per pixel
        8,  // BC1_RGB_UNORM
        8,  // BC1_RGB_SRGB
        8,  // BC1_RGBA_UNORM
        8,  // BC1_RGBA_SRGB
        16, // BC2_UNORM
        16, // BC2_SRGB
        16, // BC3_UNORM
        16, // BC3_SRGB
        8,  // BC4_UNORM
        8,  // BC4_SNORM
        16, // BC5_UNORM
        16, // BC5_SNORM
        16, // BC6H_UFLOAT
        16, // BC6H_SFLOAT
        16, // BC7_UNORM
        16, // BC7_SRGB
    };

    PG_ASSERT( Underlying( format ) < Underlying( PixelFormat::NUM_PIXEL_FORMATS ) );
    static_assert( ARRAY_COUNT( size ) == Underlying( PixelFormat::NUM_PIXEL_FORMATS ) );

    return size[static_cast<i32>( format )];
}

u32 NumBytesPerChannel( PixelFormat format ) { return NumBytesPerPixel( format ) / NumChannelsInPixelFromat( format ); }

bool PixelFormatIsNormalized( PixelFormat format )
{
    bool isNormalized = false;
    switch ( format )
    {
    case PixelFormat::R8_UNORM:
    case PixelFormat::R8_SNORM:
    case PixelFormat::R8_SRGB:

    case PixelFormat::R8_G8_UNORM:
    case PixelFormat::R8_G8_SNORM:
    case PixelFormat::R8_G8_SRGB:

    case PixelFormat::R8_G8_B8_UNORM:
    case PixelFormat::R8_G8_B8_SNORM:
    case PixelFormat::R8_G8_B8_SRGB:

    case PixelFormat::B8_G8_R8_UNORM:
    case PixelFormat::B8_G8_R8_SNORM:
    case PixelFormat::B8_G8_R8_SRGB:

    case PixelFormat::R8_G8_B8_A8_UNORM:
    case PixelFormat::R8_G8_B8_A8_SNORM:
    case PixelFormat::R8_G8_B8_A8_SRGB:

    case PixelFormat::B8_G8_R8_A8_UNORM:
    case PixelFormat::B8_G8_R8_A8_SNORM:
    case PixelFormat::B8_G8_R8_A8_SRGB:

    case PixelFormat::R16_UNORM:
    case PixelFormat::R16_SNORM:

    case PixelFormat::R16_G16_UNORM:
    case PixelFormat::R16_G16_SNORM:

    case PixelFormat::R16_G16_B16_UNORM:
    case PixelFormat::R16_G16_B16_SNORM:

    case PixelFormat::R16_G16_B16_A16_UNORM:
    case PixelFormat::R16_G16_B16_A16_SNORM: isNormalized = true;
    }

    return isNormalized;
}

bool PixelFormatIsUnsigned( PixelFormat format )
{
    bool isUnsigned = false;
    switch ( format )
    {
    case PixelFormat::R8_UNORM:
    case PixelFormat::R8_UINT:
    case PixelFormat::R8_SRGB:

    case PixelFormat::R8_G8_UNORM:
    case PixelFormat::R8_G8_UINT:
    case PixelFormat::R8_G8_SRGB:

    case PixelFormat::R8_G8_B8_UNORM:
    case PixelFormat::R8_G8_B8_UINT:
    case PixelFormat::R8_G8_B8_SRGB:

    case PixelFormat::B8_G8_R8_UNORM:
    case PixelFormat::B8_G8_R8_UINT:
    case PixelFormat::B8_G8_R8_SRGB:

    case PixelFormat::R8_G8_B8_A8_UNORM:
    case PixelFormat::R8_G8_B8_A8_UINT:
    case PixelFormat::R8_G8_B8_A8_SRGB:

    case PixelFormat::B8_G8_R8_A8_UNORM:
    case PixelFormat::B8_G8_R8_A8_UINT:
    case PixelFormat::B8_G8_R8_A8_SRGB:

    case PixelFormat::R16_UNORM:
    case PixelFormat::R16_UINT:

    case PixelFormat::R16_G16_UNORM:
    case PixelFormat::R16_G16_UINT:

    case PixelFormat::R16_G16_B16_UNORM:
    case PixelFormat::R16_G16_B16_UINT:

    case PixelFormat::R16_G16_B16_A16_UNORM:
    case PixelFormat::R16_G16_B16_A16_UINT:

    case PixelFormat::R32_UINT:
    case PixelFormat::R32_G32_UINT:
    case PixelFormat::R32_G32_B32_UINT:
    case PixelFormat::R32_G32_B32_A32_UINT: isUnsigned = true;
    }

    return isUnsigned;
}

bool PixelFormatIsSrgb( PixelFormat format )
{
    bool isSrgb = false;
    switch ( format )
    {
    case PixelFormat::R8_SRGB:
    case PixelFormat::R8_G8_SRGB:
    case PixelFormat::R8_G8_B8_SRGB:
    case PixelFormat::B8_G8_R8_SRGB:
    case PixelFormat::R8_G8_B8_A8_SRGB:
    case PixelFormat::B8_G8_R8_A8_SRGB: isSrgb = true;
    }

    return isSrgb;
}

bool PixelFormatIsFloat16( PixelFormat format )
{
    auto f = Underlying( format );
    return Underlying( PixelFormat::R16_FLOAT ) <= f && f <= Underlying( PixelFormat::R16_G16_B16_A16_FLOAT );
}

bool PixelFormatIsFloat32( PixelFormat format )
{
    auto f = Underlying( format );
    return Underlying( PixelFormat::R32_FLOAT ) <= f && f <= Underlying( PixelFormat::R32_G32_B32_A32_FLOAT );
}

bool PixelFormatIsFloat( PixelFormat format ) { return PixelFormatIsFloat16( format ) || PixelFormatIsFloat32( format ); }

bool PixelFormatHasDepthFormat( PixelFormat format )
{
    auto f = Underlying( format );
    return Underlying( PixelFormat::DEPTH_16_UNORM ) <= f && f <= Underlying( PixelFormat::DEPTH_32_FLOAT_STENCIL_8_UINT );
}

bool PixelFormatHasStencil( PixelFormat format )
{
    auto f = Underlying( format );
    return Underlying( PixelFormat::DEPTH_16_UNORM_STENCIL_8_UINT ) <= f && f <= Underlying( PixelFormat::STENCIL_8_UINT );
}

bool PixelFormatIsDepthOnly( PixelFormat format ) { return format == PixelFormat::DEPTH_16_UNORM || format == PixelFormat::DEPTH_32_FLOAT; }

bool PixelFormatIsCompressed( PixelFormat format )
{
    auto f = Underlying( format );
    return Underlying( PixelFormat::BC1_RGB_UNORM ) <= f && f < Underlying( PixelFormat::NUM_PIXEL_FORMATS );
}

void GetRGBAOrder( PixelFormat format, i32 channelRemap[4] )
{
    channelRemap[0] = 0;
    channelRemap[1] = 1;
    channelRemap[2] = 2;
    channelRemap[3] = 3;
    switch ( format )
    {
    case PixelFormat::B8_G8_R8_UNORM:
    case PixelFormat::B8_G8_R8_SNORM:
    case PixelFormat::B8_G8_R8_UINT:
    case PixelFormat::B8_G8_R8_SINT:
    case PixelFormat::B8_G8_R8_SRGB:
    case PixelFormat::B8_G8_R8_A8_UNORM:
    case PixelFormat::B8_G8_R8_A8_SNORM:
    case PixelFormat::B8_G8_R8_A8_UINT:
    case PixelFormat::B8_G8_R8_A8_SINT:
    case PixelFormat::B8_G8_R8_A8_SRGB: channelRemap[2] = 0; channelRemap[0] = 2;
    }
}

std::string PixelFormatName( PixelFormat format )
{
    const char* names[] = {
        "INVALID",                       // INVALID
        "R8_UNORM",                      // R8_UNORM
        "R8_SNORM",                      // R8_SNORM
        "R8_UINT",                       // R8_UINT
        "R8_SINT",                       // R8_SINT
        "R8_SRGB",                       // R8_SRGB
        "R8_G8_UNORM",                   // R8_G8_UNORM
        "R8_G8_SNORM",                   // R8_G8_SNORM
        "R8_G8_UINT",                    // R8_G8_UINT
        "R8_G8_SINT",                    // R8_G8_SINT
        "R8_G8_SRGB",                    // R8_G8_SRGB
        "R8_G8_B8_UNORM",                // R8_G8_B8_UNORM
        "R8_G8_B8_SNORM",                // R8_G8_B8_SNORM
        "R8_G8_B8_UINT",                 // R8_G8_B8_UINT
        "R8_G8_B8_SINT",                 // R8_G8_B8_SINT
        "R8_G8_B8_SRGB",                 // R8_G8_B8_SRGB
        "B8_G8_R8_UNORM",                // B8_G8_R8_UNORM
        "B8_G8_R8_SNORM",                // B8_G8_R8_SNORM
        "B8_G8_R8_UINT",                 // B8_G8_R8_UINT
        "B8_G8_R8_SINT",                 // B8_G8_R8_SINT
        "B8_G8_R8_SRGB",                 // B8_G8_R8_SRGB
        "R8_G8_B8_A8_UNORM",             // R8_G8_B8_A8_UNORM
        "R8_G8_B8_A8_SNORM",             // R8_G8_B8_A8_SNORM
        "R8_G8_B8_A8_UINT",              // R8_G8_B8_A8_UINT
        "R8_G8_B8_A8_SINT",              // R8_G8_B8_A8_SINT
        "R8_G8_B8_A8_SRGB",              // R8_G8_B8_A8_SRGB
        "B8_G8_R8_A8_UNORM",             // B8_G8_R8_A8_UNORM
        "B8_G8_R8_A8_SNORM",             // B8_G8_R8_A8_SNORM
        "B8_G8_R8_A8_UINT",              // B8_G8_R8_A8_UINT
        "B8_G8_R8_A8_SINT",              // B8_G8_R8_A8_SINT
        "B8_G8_R8_A8_SRGB",              // B8_G8_R8_A8_SRGB
        "R16_UNORM",                     // R16_UNORM
        "R16_SNORM",                     // R16_SNORM
        "R16_UINT",                      // R16_UINT
        "R16_SINT",                      // R16_SINT
        "R16_FLOAT",                     // R16_FLOAT
        "R16_G16_UNORM",                 // R16_G16_UNORM
        "R16_G16_SNORM",                 // R16_G16_SNORM
        "R16_G16_UINT",                  // R16_G16_UINT
        "R16_G16_SINT",                  // R16_G16_SINT
        "R16_G16_FLOAT",                 // R16_G16_FLOAT
        "R16_G16_B16_UNORM",             // R16_G16_B16_UNORM
        "R16_G16_B16_SNORM",             // R16_G16_B16_SNORM
        "R16_G16_B16_UINT",              // R16_G16_B16_UINT
        "R16_G16_B16_SINT",              // R16_G16_B16_SINT
        "R16_G16_B16_FLOAT",             // R16_G16_B16_FLOAT
        "R16_G16_B16_A16_UNORM",         // R16_G16_B16_A16_UNORM
        "R16_G16_B16_A16_SNORM",         // R16_G16_B16_A16_SNORM
        "R16_G16_B16_A16_UINT",          // R16_G16_B16_A16_UINT
        "R16_G16_B16_A16_SINT",          // R16_G16_B16_A16_SINT
        "R16_G16_B16_A16_FLOAT",         // R16_G16_B16_A16_FLOAT
        "R32_UINT",                      // R32_UINT
        "R32_SINT",                      // R32_SINT
        "R32_FLOAT",                     // R32_FLOAT
        "R32_G32_UINT",                  // R32_G32_UINT
        "R32_G32_SINT",                  // R32_G32_SINT
        "R32_G32_FLOAT",                 // R32_G32_FLOAT
        "R32_G32_B32_UINT",              // R32_G32_B32_UINT
        "R32_G32_B32_SINT",              // R32_G32_B32_SINT
        "R32_G32_B32_FLOAT",             // R32_G32_B32_FLOAT
        "R32_G32_B32_A32_UINT",          // R32_G32_B32_A32_UINT
        "R32_G32_B32_A32_SINT",          // R32_G32_B32_A32_SINT
        "R32_G32_B32_A32_FLOAT",         // R32_G32_B32_A32_FLOAT
        "DEPTH_16_UNORM",                // DEPTH_16_UNORM
        "DEPTH_32_FLOAT",                // DEPTH_32_FLOAT
        "DEPTH_16_UNORM_STENCIL_8_UINT", // DEPTH_16_UNORM_STENCIL_8_UINT
        "DEPTH_24_UNORM_STENCIL_8_UINT", // DEPTH_24_UNORM_STENCIL_8_UINT
        "DEPTH_32_FLOAT_STENCIL_8_UINT", // DEPTH_32_FLOAT_STENCIL_8_UINT
        "STENCIL_8_UINT",                // STENCIL_8_UINT
        "BC1_RGB_UNORM",                 // BC1_RGB_UNORM
        "BC1_RGB_SRGB",                  // BC1_RGB_SRGB
        "BC1_RGBA_UNORM",                // BC1_RGBA_UNORM
        "BC1_RGBA_SRGB",                 // BC1_RGBA_SRGB
        "BC2_UNORM",                     // BC2_UNORM
        "BC2_SRGB",                      // BC2_SRGB
        "BC3_UNORM",                     // BC3_UNORM
        "BC3_SRGB",                      // BC3_SRGB
        "BC4_UNORM",                     // BC4_UNORM
        "BC4_SNORM",                     // BC4_SNORM
        "BC5_UNORM",                     // BC5_UNORM
        "BC5_SNORM",                     // BC5_SNORM
        "BC6H_UFLOAT",                   // BC6H_UFLOAT
        "BC6H_SFLOAT",                   // BC6H_SFLOAT
        "BC7_UNORM",                     // BC7_UNORM
        "BC7_SRGB",                      // BC7_SRGB
    };

    return names[Underlying( format )];
}

PixelFormat PixelFormatFromString( const std::string& format )
{
    std::unordered_map<std::string, PixelFormat> pixelFormatMap = {
        {"R8_UNORM",                      PixelFormat::R8_UNORM                     },
        {"R8_SNORM",                      PixelFormat::R8_SNORM                     },
        {"R8_UINT",                       PixelFormat::R8_UINT                      },
        {"R8_SINT",                       PixelFormat::R8_SINT                      },
        {"R8_SRGB",                       PixelFormat::R8_SRGB                      },
        {"R8_G8_UNORM",                   PixelFormat::R8_G8_UNORM                  },
        {"R8_G8_SNORM",                   PixelFormat::R8_G8_SNORM                  },
        {"R8_G8_UINT",                    PixelFormat::R8_G8_UINT                   },
        {"R8_G8_SINT",                    PixelFormat::R8_G8_SINT                   },
        {"R8_G8_SRGB",                    PixelFormat::R8_G8_SRGB                   },
        {"R8_G8_B8_UNORM",                PixelFormat::R8_G8_B8_UNORM               },
        {"R8_G8_B8_SNORM",                PixelFormat::R8_G8_B8_SNORM               },
        {"R8_G8_B8_UINT",                 PixelFormat::R8_G8_B8_UINT                },
        {"R8_G8_B8_SINT",                 PixelFormat::R8_G8_B8_SINT                },
        {"R8_G8_B8_SRGB",                 PixelFormat::R8_G8_B8_SRGB                },
        {"B8_G8_R8_UNORM",                PixelFormat::B8_G8_R8_UNORM               },
        {"B8_G8_R8_SNORM",                PixelFormat::B8_G8_R8_SNORM               },
        {"B8_G8_R8_UINT",                 PixelFormat::B8_G8_R8_UINT                },
        {"B8_G8_R8_SINT",                 PixelFormat::B8_G8_R8_SINT                },
        {"B8_G8_R8_SRGB",                 PixelFormat::B8_G8_R8_SRGB                },
        {"R8_G8_B8_A8_UNORM",             PixelFormat::R8_G8_B8_A8_UNORM            },
        {"R8_G8_B8_A8_SNORM",             PixelFormat::R8_G8_B8_A8_SNORM            },
        {"R8_G8_B8_A8_UINT",              PixelFormat::R8_G8_B8_A8_UINT             },
        {"R8_G8_B8_A8_SINT",              PixelFormat::R8_G8_B8_A8_SINT             },
        {"R8_G8_B8_A8_SRGB",              PixelFormat::R8_G8_B8_A8_SRGB             },
        {"B8_G8_R8_A8_UNORM",             PixelFormat::B8_G8_R8_A8_UNORM            },
        {"B8_G8_R8_A8_SNORM",             PixelFormat::B8_G8_R8_A8_SNORM            },
        {"B8_G8_R8_A8_UINT",              PixelFormat::B8_G8_R8_A8_UINT             },
        {"B8_G8_R8_A8_SINT",              PixelFormat::B8_G8_R8_A8_SINT             },
        {"B8_G8_R8_A8_SRGB",              PixelFormat::B8_G8_R8_A8_SRGB             },
        {"R16_UNORM",                     PixelFormat::R16_UNORM                    },
        {"R16_SNORM",                     PixelFormat::R16_SNORM                    },
        {"R16_UINT",                      PixelFormat::R16_UINT                     },
        {"R16_SINT",                      PixelFormat::R16_SINT                     },
        {"R16_FLOAT",                     PixelFormat::R16_FLOAT                    },
        {"R16_G16_UNORM",                 PixelFormat::R16_G16_UNORM                },
        {"R16_G16_SNORM",                 PixelFormat::R16_G16_SNORM                },
        {"R16_G16_UINT",                  PixelFormat::R16_G16_UINT                 },
        {"R16_G16_SINT",                  PixelFormat::R16_G16_SINT                 },
        {"R16_G16_FLOAT",                 PixelFormat::R16_G16_FLOAT                },
        {"R16_G16_B16_UNORM",             PixelFormat::R16_G16_B16_UNORM            },
        {"R16_G16_B16_SNORM",             PixelFormat::R16_G16_B16_SNORM            },
        {"R16_G16_B16_UINT",              PixelFormat::R16_G16_B16_UINT             },
        {"R16_G16_B16_SINT",              PixelFormat::R16_G16_B16_SINT             },
        {"R16_G16_B16_FLOAT",             PixelFormat::R16_G16_B16_FLOAT            },
        {"R16_G16_B16_A16_UNORM",         PixelFormat::R16_G16_B16_A16_UNORM        },
        {"R16_G16_B16_A16_SNORM",         PixelFormat::R16_G16_B16_A16_SNORM        },
        {"R16_G16_B16_A16_UINT",          PixelFormat::R16_G16_B16_A16_UINT         },
        {"R16_G16_B16_A16_SINT",          PixelFormat::R16_G16_B16_A16_SINT         },
        {"R16_G16_B16_A16_FLOAT",         PixelFormat::R16_G16_B16_A16_FLOAT        },
        {"R32_UINT",                      PixelFormat::R32_UINT                     },
        {"R32_SINT",                      PixelFormat::R32_SINT                     },
        {"R32_FLOAT",                     PixelFormat::R32_FLOAT                    },
        {"R32_G32_UINT",                  PixelFormat::R32_G32_UINT                 },
        {"R32_G32_SINT",                  PixelFormat::R32_G32_SINT                 },
        {"R32_G32_FLOAT",                 PixelFormat::R32_G32_FLOAT                },
        {"R32_G32_B32_UINT",              PixelFormat::R32_G32_B32_UINT             },
        {"R32_G32_B32_SINT",              PixelFormat::R32_G32_B32_SINT             },
        {"R32_G32_B32_FLOAT",             PixelFormat::R32_G32_B32_FLOAT            },
        {"R32_G32_B32_A32_UINT",          PixelFormat::R32_G32_B32_A32_UINT         },
        {"R32_G32_B32_A32_SINT",          PixelFormat::R32_G32_B32_A32_SINT         },
        {"R32_G32_B32_A32_FLOAT",         PixelFormat::R32_G32_B32_A32_FLOAT        },
        {"DEPTH_16_UNORM",                PixelFormat::DEPTH_16_UNORM               },
        {"DEPTH_32_FLOAT",                PixelFormat::DEPTH_32_FLOAT               },
        {"DEPTH_16_UNORM_STENCIL_8_UINT", PixelFormat::DEPTH_16_UNORM_STENCIL_8_UINT},
        {"DEPTH_24_UNORM_STENCIL_8_UINT", PixelFormat::DEPTH_24_UNORM_STENCIL_8_UINT},
        {"DEPTH_32_FLOAT_STENCIL_8_UINT", PixelFormat::DEPTH_32_FLOAT_STENCIL_8_UINT},
        {"STENCIL_8_UINT",                PixelFormat::STENCIL_8_UINT               },
        {"BC1_RGB_UNORM",                 PixelFormat::BC1_RGB_UNORM                },
        {"BC1_RGB_SRGB",                  PixelFormat::BC1_RGB_SRGB                 },
        {"BC1_RGBA_UNORM",                PixelFormat::BC1_RGBA_UNORM               },
        {"BC1_RGBA_SRGB",                 PixelFormat::BC1_RGBA_SRGB                },
        {"BC2_UNORM",                     PixelFormat::BC2_UNORM                    },
        {"BC2_SRGB",                      PixelFormat::BC2_SRGB                     },
        {"BC3_UNORM",                     PixelFormat::BC3_UNORM                    },
        {"BC3_SRGB",                      PixelFormat::BC3_SRGB                     },
        {"BC4_UNORM",                     PixelFormat::BC4_UNORM                    },
        {"BC4_SNORM",                     PixelFormat::BC4_SNORM                    },
        {"BC5_UNORM",                     PixelFormat::BC5_UNORM                    },
        {"BC5_SNORM",                     PixelFormat::BC5_SNORM                    },
        {"BC6H_UFLOAT",                   PixelFormat::BC6H_UFLOAT                  },
        {"BC6H_SFLOAT",                   PixelFormat::BC6H_SFLOAT                  },
        {"BC7_UNORM",                     PixelFormat::BC7_UNORM                    },
        {"BC7_SRGB",                      PixelFormat::BC7_SRGB                     },
    };

    auto it = pixelFormatMap.find( format );
    if ( it == pixelFormatMap.end() )
    {
        return PixelFormat::INVALID;
    }

    return it->second;
}

} // namespace PG
