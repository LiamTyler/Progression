#include "core/bounding_box.hpp"

namespace PG
{

AABB::AABB() : min( FLT_MAX ), max( -FLT_MAX ) {}

AABB::AABB( const vec3& _min, const vec3& _max ) : min( _min ), max( _max ) {}

vec3 AABB::Center() const { return 0.5f * ( max + min ); }

void AABB::Points( vec3* data ) const
{
    data[0] = vec3( min.x, min.y, min.z );
    data[1] = vec3( min.x, max.y, min.z );
    data[2] = vec3( max.x, min.y, min.z );
    data[3] = vec3( max.x, max.y, min.z );
    data[4] = vec3( min.x, min.y, max.z );
    data[5] = vec3( min.x, max.y, max.z );
    data[6] = vec3( max.x, min.y, max.z );
    data[7] = vec3( max.x, max.y, max.z );
}

i32 AABB::LongestDimension() const
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

f32 AABB::SurfaceArea() const
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

void AABB::Scale( vec3 v )
{
    min *= v;
    max *= v;
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

void AABB::Encompass( vec3 point )
{
    min = Min( min, point );
    max = Max( max, point );
}

void AABB::Encompass( vec3* points, i32 numPoints )
{
    for ( i32 i = 0; i < numPoints; ++i )
    {
        min = Min( min, points[i] );
        max = Max( max, points[i] );
    }
}

AABB AABB::Transform( const mat4& transform ) const
{
    vec3 pos = transform[3];
    AABB aabb( pos, pos );
    for ( int i = 0; i < 3; i++ )
    {
        for ( int j = 0; j < 3; j++ )
        {
            // [j][i] not [i][j] because transform is column major
            float a = transform[j][i] * min[j];
            float b = transform[j][i] * max[j];

            aabb.min[i] += Min( a, b );
            aabb.max[i] += Max( a, b );
        }
    }

    return aabb;
}

} // namespace PG
