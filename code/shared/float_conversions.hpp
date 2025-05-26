#pragma once

#include "shared/math_vec.hpp"
#include <algorithm>
#include <cstring>

using float16 = u16;

constexpr u16 FP16_ZERO = 0x0000;
constexpr u16 FP16_ONE  = 0x3C00;

// implementation from tinyexr library
union FP32
{
    u32 u;
    f32 f;
    struct
    {
        u32 Mantissa : 23;
        u32 Exponent : 8;
        u32 Sign : 1;
    };
};

union FP16
{
    u16 u;
    struct
    {
        u16 Mantissa : 10;
        u16 Exponent : 5;
        u16 Sign : 1;
    };
};

// Original ISPC reference version; this always rounds ties up.
inline u16 Float32ToFloat16( f32 f32 )
{
    FP32 f;
    f.f    = f32;
    FP16 o = { 0 };

    // Based on ISPC reference code (with minor modifications)
    if ( f.Exponent == 0 ) // Signed zero/denormal (which will underflow)
    {
        o.Exponent = 0;
    }
    else if ( f.Exponent == 255 ) // Inf or NaN (all exponent bits set)
    {
        o.Exponent = 31;
        o.Mantissa = f.Mantissa ? 0x200 : 0; // NaN->qNaN and Inf->Inf
    }
    else // Normalized number
    {
        // Exponent unbias the single, then bias the halfp
        i32 newexp = f.Exponent - 127 + 15;
        if ( newexp >= 31 ) // Overflow, return signed infinity
        {
            o.Exponent = 31;
        }
        else if ( newexp <= 0 ) // Underflow
        {
            if ( ( 14 - newexp ) <= 24 ) // Mantissa might be non-zero
            {
                u32 mant   = f.Mantissa | 0x800000; // Hidden 1 bit
                o.Mantissa = mant >> ( 14 - newexp );
                if ( ( mant >> ( 13 - newexp ) ) & 1 ) // Check for rounding
                {
                    o.u++; // Round, might overflow into exp bit, but this is OK
                }
            }
        }
        else
        {
            o.Exponent = newexp;
            o.Mantissa = f.Mantissa >> 13;
            if ( f.Mantissa & 0x1000 ) // Check for rounding
            {
                o.u++; // Round, might overflow to inf, this is OK
            }
        }
    }

    o.Sign = f.Sign;
    return o.u;
}

inline u16vec4 Float32ToFloat16( vec4 v )
{
    return { Float32ToFloat16( v.x ), Float32ToFloat16( v.y ), Float32ToFloat16( v.z ), Float32ToFloat16( v.w ) };
}

inline u32 Float32ToFloat16( f32 x, f32 y )
{
    u32 px = Float32ToFloat16( x );
    u32 py = Float32ToFloat16( y );
    return px | ( py << 16 );
}

inline f32 Float32ToFloat16AsFloat( f32 x, f32 y )
{
    u32 px      = Float32ToFloat16( x );
    u32 py      = Float32ToFloat16( y );
    uint packed = px | ( py << 16 );
    f32 ret;
    memcpy( &ret, &packed, sizeof( f32 ) );
    return ret;
}

inline f32 Float16ToFloat32( u16 f16 )
{
    static const FP32 magic      = { 113 << 23 };
    static const u32 shifted_exp = 0x7c00 << 13; // exponent mask after shift
    FP32 o;
    FP16 h;
    h.u = f16;

    o.u      = ( h.u & 0x7fffU ) << 13U; // exponent/mantissa bits
    u32 exp_ = shifted_exp & o.u;        // just the exponent
    o.u += ( 127 - 15 ) << 23;           // exponent adjust

    // handle exponent special cases
    if ( exp_ == shifted_exp ) // Inf/NaN?
    {
        o.u += ( 128 - 16 ) << 23; // extra exp adjust
    }
    else if ( exp_ == 0 ) // Zero/Denormal?
    {
        o.u += 1 << 23; // extra exp adjust
        o.f -= magic.f; // renormalize
    }

    o.u |= ( h.u & 0x8000U ) << 16U; // sign bit
    return o.f;
}

inline vec4 Float16ToFloat32( u16vec4 v )
{
    return { Float16ToFloat32( v.x ), Float16ToFloat32( v.y ), Float16ToFloat32( v.z ), Float16ToFloat32( v.w ) };
}

inline constexpr u8 UNormFloatToByte( f32 x ) { return static_cast<u8>( 255.0f * x + 0.5f ); }

inline u32 UNormFloat4ToU32( vec4 v )
{
    return ( UNormFloatToByte( v.x ) << 0 ) | ( UNormFloatToByte( v.y ) << 8 ) | ( UNormFloatToByte( v.z ) << 16 ) |
           ( UNormFloatToByte( v.w ) << 24 );
}

inline constexpr f32 UNormByteToFloat( u8 x ) { return x / 255.0f; }

inline vec4 UNormByteToFloat( u8vec4 v ) { return { UNormByteToFloat( v.x ), UNormByteToFloat( v.y ), UNormByteToFloat( v.z ), v.w }; }

inline constexpr f32 UNorm16ToFloat( u16 x ) { return f32( x ) / 65535.0f; }

inline constexpr u16 FloatToUNorm16( f32 x ) { return static_cast<u16>( 65535.0f * x + 0.5f ); }
