#pragma once

#include "shared/core_defines.hpp"
#include <algorithm>

#define PI 3.14159265358979323846f
#define PI_D 3.14159265358979323846

inline constexpr f32 Saturate( const f32 x ) { return std::clamp( x, 0.0f, 1.0f ); }

inline constexpr f32 DegToRad( const f32 x ) { return x * ( PI / 180.0f ); }

inline constexpr f32 RadToDeg( const f32 x ) { return x * ( 180.0f / PI ); }

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

#define ROUND_UP_TO_MULT( x, m ) ( ( ( x + ( m - 1 ) ) / m ) * m )
