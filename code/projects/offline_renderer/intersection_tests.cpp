#include "intersection_tests.hpp"
#include <algorithm>
#include <cmath>

namespace PT::intersect
{

bool RayPlaneIntersection( const vec3& rayPos, const vec3& rayDir, const vec3& N, const vec3& arbitraryPointOnPlane, f32& t )
{
    f32 denom = Dot( N, rayDir );
    if ( fabs( denom ) < 1e-6f )
    {
        return false;
    }

    f32 d = Dot( N, arbitraryPointOnPlane );
    t     = -( Dot( N, rayPos ) - d ) / denom;
    return t > 0;
}

bool RaySphere( const vec3& rayPos, const vec3& rayDir, const vec3& spherePos, f32 radius, f32& t, f32 maxT )
{
    vec3 OC = rayPos - spherePos;
    f32 a   = Dot( rayDir, rayDir );
    f32 b   = 2 * Dot( OC, rayDir );
    f32 c   = Dot( OC, OC ) - radius * radius;

    f32 disc = b * b - 4 * a * c;
    if ( disc < 0 )
    {
        return false;
    }

    f32 d = std::sqrt( disc );
    t     = ( -b - d ) / ( 2 * a );
    if ( t < 0 )
    {
        t = ( -b + d ) / ( 2 * a );
    }

    return t < maxT && t > 0;
}

// https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/moller-trumbore-ray-triangle-intersection
bool RayTriangle( const vec3& rayPos, const vec3& rayDir, const vec3& v0, const vec3& v1, const vec3& v2, f32& t, f32& u, f32& v, f32 maxT )
{
    vec3 v0v1 = v1 - v0;
    vec3 v0v2 = v2 - v0;
    vec3 pvec = Cross( rayDir, v0v2 );
    f32 det   = Dot( v0v1, pvec );
    // ray and triangle are parallel if det is close to 0
    if ( fabs( det ) < 0.000000001 )
    {
        return false;
    }

    f32 invDet = 1 / det;

    vec3 tvec = rayPos - v0;
    u         = Dot( tvec, pvec ) * invDet;
    if ( u < 0 || u > 1 )
    {
        return false;
    }

    vec3 qvec = Cross( tvec, v0v1 );
    v         = Dot( rayDir, qvec ) * invDet;
    if ( v < 0 || u + v > 1 )
    {
        return false;
    }

    t = Dot( v0v2, qvec ) * invDet;

    return t < maxT && t > 0;
}

bool RayAABB( const vec3& rayPos, const vec3& invRayDir, const vec3& aabbMin, const vec3& aabbMax, f32 maxT )
{
    f32 tx1 = ( aabbMin.x - rayPos.x ) * invRayDir.x;
    f32 tx2 = ( aabbMax.x - rayPos.x ) * invRayDir.x;

    f32 tmin = Min( tx1, tx2 );
    f32 tmax = Max( tx1, tx2 );

    f32 ty1 = ( aabbMin.y - rayPos.y ) * invRayDir.y;
    f32 ty2 = ( aabbMax.y - rayPos.y ) * invRayDir.y;

    tmin = Max( tmin, Min( ty1, ty2 ) );
    tmax = Min( tmax, Max( ty1, ty2 ) );

    f32 tz1 = ( aabbMin.z - rayPos.z ) * invRayDir.z;
    f32 tz2 = ( aabbMax.z - rayPos.z ) * invRayDir.z;

    tmin = Max( tmin, Min( tz1, tz2 ) );
    tmax = Min( tmax, Max( tz1, tz2 ) );

    return ( tmin < maxT ) && ( tmax >= Max( 0.0f, tmin ) );
}

bool RayAABBFastest( const vec3& rayPos, const vec3& invRayDir, const i32 isDirNeg[3], const vec3& aabbMin, const vec3& aabbMax, f32 maxT )
{
    f32 tMin  = ( ( isDirNeg[0] ? aabbMax : aabbMin ).x - rayPos.x ) * invRayDir.x;
    f32 tMax  = ( ( 1 - isDirNeg[0] ? aabbMax : aabbMin ).x - rayPos.x ) * invRayDir.x;
    f32 tyMin = ( ( isDirNeg[1] ? aabbMax : aabbMin ).y - rayPos.y ) * invRayDir.y;
    f32 tyMax = ( ( 1 - isDirNeg[1] ? aabbMax : aabbMin ).y - rayPos.y ) * invRayDir.y;

    if ( tMin > tyMax || tyMin > tMax )
        return false;
    if ( tyMin > tMin )
        tMin = tyMin;
    if ( tyMax < tMax )
        tMax = tyMax;

    f32 tzMin = ( ( isDirNeg[2] ? aabbMax : aabbMin ).z - rayPos.z ) * invRayDir.z;
    f32 tzMax = ( ( 1 - isDirNeg[2] ? aabbMax : aabbMin ).z - rayPos.z ) * invRayDir.z;

    if ( tMin > tzMax || tzMin > tMax )
        return false;
    if ( tzMin > tMin )
        tMin = tzMin;
    if ( tzMax < tMax )
        tMax = tzMax;
    return ( tMin < maxT ) && ( tMax > 0 );
}

} // namespace PT::intersect
