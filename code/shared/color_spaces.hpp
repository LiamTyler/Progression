#pragma once

#include "glm/vec4.hpp"
#include <cmath>

namespace PG
{

inline float LinearToGammaSRGB( float x )
{
    if ( x <= 0.0031308f )
    {
        return 12.92f * x;
    }
    else
    {
        return 1.055f * std::pow( x, 1.0f / 2.4f ) - 0.055f;
    }
}


inline glm::vec4 LinearToGammaSRGB( glm::vec4 v )
{
    return
    {
        LinearToGammaSRGB( v.x ),
        LinearToGammaSRGB( v.y ),
        LinearToGammaSRGB( v.z ),
        v.w
    };  
}


inline float GammaSRGBToLinear( float x )
{
    if ( x <= 0.04045f )
    {
        return x / 12.92f;
    }
    else
    {
        return std::pow( (x + 0.055f) / 1.055f, 2.4f );
    }
}


inline glm::vec4 GammaSRGBToLinear( glm::vec4 v )
{
    return
    {
        GammaSRGBToLinear( v.x ),
        GammaSRGBToLinear( v.y ),
        GammaSRGBToLinear( v.z ),
        v.w
    };  
}

} // namespace PG
