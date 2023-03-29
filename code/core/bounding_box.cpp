#include "core/bounding_box.hpp"
#include <cfloat>

namespace PG
{

AABB::AABB() : min( FLT_MAX ), max( -FLT_MAX ) {}

AABB::AABB( const glm::vec3& _min, const glm::vec3& _max ) : min( _min ), max( _max ) {}


glm::vec3 AABB::Center() const
{
    return 0.5f * ( max + min );
}


void AABB::Points( glm::vec3* data ) const
{
    glm::vec3 extent = max - min;
    data[0] = min;
    data[1] = min + glm::vec3( extent.x, 0, 0 );
    data[2] = min + glm::vec3( extent.x, 0, extent.z );
    data[3] = min + glm::vec3( 0, 0, extent.z );
    data[4] = max;
    data[5] = max - glm::vec3( extent.x, 0, 0 );
    data[6] = max - glm::vec3( extent.x, 0, extent.z );
    data[7] = max - glm::vec3( 0, 0, extent.z );
}


int AABB::LongestDimension() const
{
    glm::vec3 d = max - min;
    if ( d[0] > d[1] && d[0] > d[2] )
    {
        return 0;
    }
    else if ( d[1] > d[2] )
    {
        return 1;
    }
    else
    {
        return 2;
    }
}


float AABB::SurfaceArea() const
{
    glm::vec3 d = max - min;
    return 2 * (d.x*d.y + d.x*d.z + d.y*d.z);
}


glm::mat4 AABB::ModelMatrix() const
{
    glm::vec3 center = .5f * (min + max);
    glm::vec3 scale  = 0.5f * (max - min);
    glm::mat4 model = glm::mat4(
        scale.x, 0, 0, 0,
        0, scale.y, 0, 0,
        0, 0, scale.z, 0,
        center.x, center.y, center.z, 1
    );
    return model;
}


glm::vec3 AABB::P( const glm::vec3& planeNormal ) const
{
    glm::vec3 P = min;
    if ( planeNormal.x >= 0 ) P.x = max.x;
    if ( planeNormal.y >= 0 ) P.y = max.y;
    if ( planeNormal.z >= 0 ) P.z = max.z;

    return P;
}


glm::vec3 AABB::N( const glm::vec3& planeNormal ) const
{
    glm::vec3 N = max;
    if ( planeNormal.x >= 0 ) N.x = min.x;
    if ( planeNormal.y >= 0 ) N.y = min.y;
    if ( planeNormal.z >= 0 ) N.z = min.z;

    return N;
}


glm::vec3 AABB::Offset( const glm::vec3& p ) const
{
    glm::vec3 d   = max - min;
    glm::vec3 rel = p - min;
    if ( max.x > min.x ) rel.x /= d.x;
    if ( max.y > min.y ) rel.y /= d.y;
    if ( max.z > min.z ) rel.z /= d.z;
    return rel;
}


void AABB::MoveCenterTo( const glm::vec3& point )
{
    glm::vec3 halfExtent = 0.5f * (max - min);
    min = point - halfExtent;
    max = point + halfExtent;
}


void AABB::Encompass( const AABB& aabb )
{
    min = glm::min( min, aabb.min );
    max = glm::max( max, aabb.max );
}


void AABB::Encompass( const AABB& aabb, const glm::mat4& transform )
{
    glm::vec3 points[8];
    aabb.Points( points );
    glm::vec3 halfExt = 0.5f * (aabb.max - aabb.min);
    for ( int i = 0; i < 8; ++i )
    {
        glm::vec3 tmp = glm::vec3( transform * glm::vec4( points[i] - halfExt, 1 ) ) + halfExt;
        min = glm::min( min, tmp );
        max = glm::max( max, tmp );
    }
}


void AABB::Encompass( glm::vec3 point )
{
    min = glm::min( min, point );
    max = glm::max( max, point );
}


void AABB::Encompass( glm::vec3* points, int numPoints )
{
    for ( int i = 0; i < numPoints; ++i )
    {
        min = glm::min( min, points[i] );
        max = glm::max( max, points[i] );
    }
}


} // namespace PG
