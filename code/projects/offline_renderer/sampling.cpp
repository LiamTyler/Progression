#include "sampling.hpp"
#include <algorithm>

namespace PT
{

glm::vec2 UniformSampleDisk( float u1, float u2, float radius )
{
    float r     = radius * std::sqrt( u1 );
    float theta = 2 * PI * u2;
    return r * glm::vec2( std::cos( theta ), std::sin( theta ) );
}

// http://l2program.co.uk/900/concentric-disk-sampling
glm::vec2 ConcentricSampleDisk( float u1, float u2, float radius )
{
    u1 = 2 * u1 - 1;
    u2 = 2 * u2 - 1;

    if ( u1 == 0 && u2 == 0 )
    {
        return { 0, 0 };
    }

    float theta = 0;
    float r     = radius;
    if ( u1*u1 > u2*u2 )
    {
        r *= u1;
        theta = (PI/4) * (u2/u1);
    }
    else
    {
        r *= u2;
        theta = (PI/2) - (PI/4) * (u1/u2);
    }
    return r * glm::vec2( std::cos( theta ), std::sin( theta ) );
}

glm::vec3 UniformSampleHemisphere( float u1, float u2 )
{
    float z     = u1;
    float r     = std::sqrt( std::max( 0.f, 1 - z * z ) );
    float theta = 2 * PI * u2;
    return glm::vec3( r * std::cos( theta ), r * std::sin( theta ), z );
}

glm::vec3 CosineSampleHemisphere( float u1, float u2 )
{
    auto d  = ConcentricSampleDisk( u1, u2 );
    float z = std::sqrt( std::max( 0.f, 1 - d.x*d.x - d.y*d.y) );
    return { d.x, d.y, z };
}

glm::vec3 UniformSampleSphere( float u1, float u2 )
{
    float z     = 1 - 2 * u1;
    float r     = std::sqrt( std::max( 0.f, 1 - z * z ) );
    float theta = 2 * PI * u2;
    return glm::vec3( r * std::cos( theta ), r * std::sin( theta ), z );
}

glm::vec2 UniformSampleTriangle( float u1, float u2 )
{
    float su0 = std::sqrt( u1 );
    return { 1 - su0, u2 * su0 };
}

} // namespace PT