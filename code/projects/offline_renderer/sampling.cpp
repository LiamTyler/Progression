#include "sampling.hpp"
#include <algorithm>

namespace PT
{

vec2 UniformSampleDisk( f32 u1, f32 u2, f32 radius )
{
    f32 r     = radius * std::sqrt( u1 );
    f32 theta = 2 * PI * u2;
    return r * vec2( std::cos( theta ), std::sin( theta ) );
}

// http://l2program.co.uk/900/concentric-disk-sampling
vec2 ConcentricSampleDisk( f32 u1, f32 u2, f32 radius )
{
    u1 = 2 * u1 - 1;
    u2 = 2 * u2 - 1;

    if ( u1 == 0 && u2 == 0 )
    {
        return { 0, 0 };
    }

    f32 theta = 0;
    f32 r     = radius;
    if ( u1 * u1 > u2 * u2 )
    {
        r *= u1;
        theta = ( PI / 4 ) * ( u2 / u1 );
    }
    else
    {
        r *= u2;
        theta = ( PI / 2 ) - ( PI / 4 ) * ( u1 / u2 );
    }
    return r * vec2( std::cos( theta ), std::sin( theta ) );
}

vec3 UniformSampleHemisphere( f32 u1, f32 u2 )
{
    f32 z     = u1;
    f32 r     = std::sqrt( Max( 0.f, 1 - z * z ) );
    f32 theta = 2 * PI * u2;
    return vec3( r * std::cos( theta ), r * std::sin( theta ), z );
}

vec3 CosineSampleHemisphere( f32 u1, f32 u2 )
{
    auto d = ConcentricSampleDisk( u1, u2 );
    f32 z  = std::sqrt( Max( 0.f, 1 - d.x * d.x - d.y * d.y ) );
    return { d.x, d.y, z };
}

vec3 UniformSampleSphere( f32 u1, f32 u2 )
{
    f32 z     = 1 - 2 * u1;
    f32 r     = std::sqrt( Max( 0.f, 1 - z * z ) );
    f32 theta = 2 * PI * u2;
    return vec3( r * std::cos( theta ), r * std::sin( theta ), z );
}

vec2 UniformSampleTriangle( f32 u1, f32 u2 )
{
    f32 su0 = std::sqrt( u1 );
    return { 1 - su0, u2 * su0 };
}

} // namespace PT
