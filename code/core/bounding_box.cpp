#include "core/bounding_box.hpp"
#include <cfloat>

namespace PG
{

AABB::AABB() : min( FLT_MAX ), max( -FLT_MAX ) {}

AABB::AABB( const vec3& _min, const vec3& _max ) : min( _min ), max( _max ) {}

vec3 AABB::Center() const { return 0.5f * ( max + min ); }

void AABB::Points( vec3* data ) const
{
    vec3 extent = max - min;
    data[0]     = min;
    data[1]     = min + vec3( extent.x, 0, 0 );
    data[2]     = min + vec3( extent.x, 0, extent.z );
    data[3]     = min + vec3( 0, 0, extent.z );
    data[4]     = max;
    data[5]     = max - vec3( extent.x, 0, 0 );
    data[6]     = max - vec3( extent.x, 0, extent.z );
    data[7]     = max - vec3( 0, 0, extent.z );
}

int AABB::LongestDimension() const
{
    vec3 d = max - min;
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
    vec3 d = max - min;
    return 2 * ( d.x * d.y + d.x * d.z + d.y * d.z );
}

mat4 AABB::ModelMatrix() const
{
    vec3 center = .5f * ( min + max );
    vec3 scale  = 0.5f * ( max - min );
    mat4 model  = mat4( scale.x, 0, 0, 0, 0, scale.y, 0, 0, 0, 0, scale.z, 0, center.x, center.y, center.z, 1 );
    return model;
}

vec3 AABB::P( const vec3& planeNormal ) const
{
    vec3 P = min;
    if ( planeNormal.x >= 0 )
        P.x = max.x;
    if ( planeNormal.y >= 0 )
        P.y = max.y;
    if ( planeNormal.z >= 0 )
        P.z = max.z;

    return P;
}

vec3 AABB::N( const vec3& planeNormal ) const
{
    vec3 N = max;
    if ( planeNormal.x >= 0 )
        N.x = min.x;
    if ( planeNormal.y >= 0 )
        N.y = min.y;
    if ( planeNormal.z >= 0 )
        N.z = min.z;

    return N;
}

vec3 AABB::Offset( const vec3& p ) const
{
    vec3 d   = max - min;
    vec3 rel = p - min;
    if ( max.x > min.x )
        rel.x /= d.x;
    if ( max.y > min.y )
        rel.y /= d.y;
    if ( max.z > min.z )
        rel.z /= d.z;
    return rel;
}

void AABB::MoveCenterTo( const vec3& point )
{
    vec3 halfExtent = 0.5f * ( max - min );
    min             = point - halfExtent;
    max             = point + halfExtent;
}

void AABB::Encompass( const AABB& aabb )
{
    min = Min( min, aabb.min );
    max = Max( max, aabb.max );
}

void AABB::Encompass( const AABB& aabb, const mat4& transform )
{
    vec3 points[8];
    aabb.Points( points );
    vec3 halfExt = 0.5f * ( aabb.max - aabb.min );
    for ( int i = 0; i < 8; ++i )
    {
        vec3 tmp = vec3( transform * vec4( points[i] - halfExt, 1 ) ) + halfExt;
        min      = Min( min, tmp );
        max      = Max( max, tmp );
    }
}

void AABB::Encompass( vec3 point )
{
    min = Min( min, point );
    max = Max( max, point );
}

void AABB::Encompass( vec3* points, int numPoints )
{
    for ( int i = 0; i < numPoints; ++i )
    {
        min = Min( min, points[i] );
        max = Max( max, points[i] );
    }
}

} // namespace PG
