#ifndef __CULLING_GLSL__
#define __CULLING_GLSL__

#include "c_shared/cull_data.h"

// get furthest point along the normal vector
vec3 GetP( const AABB aabb, const vec3 planeNormal )
{
    vec3 p;
    p.x = planeNormal.x >= 0 ? aabb.max.x : aabb.min.x;
    p.y = planeNormal.y >= 0 ? aabb.max.y : aabb.min.y;
    p.z = planeNormal.z >= 0 ? aabb.max.z : aabb.min.z;
    return p;
}

bool IsInFrustum( const AABB aabb )
{
    int shouldCull = 0;
    for ( int i = 0; i < 6; ++i )
    {
        vec4 plane = globals.frustumPlanes[i];
        vec3 p = GetP( aabb, plane.xyz );
        shouldCull += int( dot( vec4( p, 1 ), plane ) < 0 );
    }
    
    return shouldCull == 0;
}

bool IsSphereInFrustum( vec3 center, float radius )
{
    int shouldCull = 0;
    for ( int i = 0; i < 6; ++i )
    {
        vec4 plane = globals.frustumPlanes[i];
        float dist = dot( vec4( center, 1 ), plane );
        shouldCull += int( dist < -radius );
    }
    
    return shouldCull == 0;
}

#endif // #ifndef __CULLING_GLSL__