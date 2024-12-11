#pragma once

#include "shared/core_defines.hpp"
#include <algorithm>

#define PI 3.14159265358979323846f
#define PI_D 3.14159265358979323846

inline constexpr f32 Saturate( const f32 x ) { return std::clamp( x, 0.0f, 1.0f ); }
inline constexpr f64 Saturate( const f64 x ) { return std::clamp( x, 0.0, 1.0 ); }

inline constexpr f32 DegToRad( const f32 x ) { return x * ( PI / 180.0f ); }
inline constexpr f64 DegToRad( const f64 x ) { return x * ( PI_D / 180.0 ); }

inline constexpr f32 RadToDeg( const f32 x ) { return x * ( 180.0f / PI ); }
inline constexpr f64 RadToDeg( const f64 x ) { return x * ( 180.0 / PI_D ); }

inline i32 Abs( const i32 x ) { return std::abs( x ); }
inline f32 Abs( const f32 x ) { return std::abs( x ); }
inline f64 Abs( const f64 x ) { return std::abs( x ); }

template <typename T>
constexpr T Min( const T& a, const T& b )
{
    return std::min( a, b );
}

template <typename T>
constexpr T Max( const T& a, const T& b )
{
    return std::max( a, b );
}

template <typename T>
constexpr T Clamp( const T& a, const T& lower, const T& upper )
{
    return std::clamp( a, lower, upper );
}

inline u32 QuantizeUNorm( f32 x, u32 bits )
{
    float m = f32( ( 1u << bits ) - 1u );
    return static_cast<u32>( Saturate( x ) * m + 0.5f );
}

inline f32 DequantizeUNorm( u32 qx, u32 bits )
{
    float m = f32( ( 1u << bits ) - 1u );
    return qx / m;
}

inline u32 QuantizeSNorm( f32 x, u32 bits )
{
    i32 m  = ( 1 << ( bits - 1 ) ) - 1;
    int qx = static_cast<int>( floor( std::clamp( x, -1.0f, 1.0f ) * m + 0.5f ) );
    return u32( qx + m );
}

inline f32 DequantizeSNorm( u32 qx, u32 bits )
{
    int m = ( 1 << ( bits - 1 ) ) - 1;
    return ( int( qx ) - m ) / float( m );
}

#define ROUND_UP_DIV( x, m ) ( ( x + ( m - 1 ) ) / m )
#define ROUND_UP_TO_MULT( x, m ) ( ( ( x + ( m - 1 ) ) / m ) * m )
