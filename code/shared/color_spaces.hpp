#pragma once

#include "glm/vec4.hpp"
#include <cmath>

namespace PG
{

inline float LinearToGammaSRGB( float x )
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


inline glm::vec3 LinearToGammaSRGB( glm::vec3 v )
{
    return { LinearToGammaSRGB( v.x ), LinearToGammaSRGB( v.y ), LinearToGammaSRGB( v.z ) };  
}


inline glm::vec4 LinearToGammaSRGB( glm::vec4 v )
{
    return { LinearToGammaSRGB( v.x ), LinearToGammaSRGB( v.y ), LinearToGammaSRGB( v.z ), v.a };  
}


inline float GammaSRGBToLinear( float x )
{
    if ( x <= 0.04045f ) [[unlikely]]
    {
        return x / 12.92f;
    }
    else
    {
        return std::pow( (x + 0.055f) / 1.055f, 2.4f );
    }
}


inline glm::vec3 GammaSRGBToLinear( glm::vec3 v )
{
    return { GammaSRGBToLinear( v.x ), GammaSRGBToLinear( v.y ), GammaSRGBToLinear( v.z ) };  
}


inline glm::vec4 GammaSRGBToLinear( glm::vec4 v )
{
    return { GammaSRGBToLinear( v.x ), GammaSRGBToLinear( v.y ), GammaSRGBToLinear( v.z ), v.w };  
}

} // namespace PG
