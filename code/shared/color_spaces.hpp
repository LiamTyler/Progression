#pragma once

#include "shared/math_vec.hpp"
#include <cmath>

namespace PG
{

inline f32 LinearToGammaSRGB( f32 x )
{
    if ( x <= 0.0031308f ) [[unlikely]]
    {
        return 12.92f * x;
    }
    else
    {
        return 1.055f * std::pow( x, 1.0f / 2.4f ) - 0.055f;
    }
}

inline vec3 LinearToGammaSRGB( vec3 v ) { return { LinearToGammaSRGB( v.x ), LinearToGammaSRGB( v.y ), LinearToGammaSRGB( v.z ) }; }

inline vec4 LinearToGammaSRGB( vec4 v ) { return { LinearToGammaSRGB( v.x ), LinearToGammaSRGB( v.y ), LinearToGammaSRGB( v.z ), v.a }; }

inline f32 GammaSRGBToLinear( f32 x )
{
    if ( x <= 0.04045f ) [[unlikely]]
    {
        return x / 12.92f;
    }
    else
    {
        return std::pow( ( x + 0.055f ) / 1.055f, 2.4f );
    }
}

inline vec3 GammaSRGBToLinear( vec3 v ) { return { GammaSRGBToLinear( v.x ), GammaSRGBToLinear( v.y ), GammaSRGBToLinear( v.z ) }; }

inline vec4 GammaSRGBToLinear( vec4 v ) { return { GammaSRGBToLinear( v.x ), GammaSRGBToLinear( v.y ), GammaSRGBToLinear( v.z ), v.w }; }

} // namespace PG
