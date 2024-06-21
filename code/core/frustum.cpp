#include "core/frustum.hpp"

namespace PG
{

void Frustum::Update(
    f32 vFov, f32 nearPlane, f32 farPlane, f32 aspect, const vec3& pos, const vec3& forward, const vec3& up, const vec3& right )
{
    vec3 ntl, ntr, nbl, nbr, ftl, ftr, fbr, fbl;
    f32 nearHeight, nearWidth, farHeight, farWidth;
    f32 angle = 0.5f * vFov;

    nearHeight = nearPlane * tanf( angle );
    farHeight  = farPlane * tanf( angle );
    nearWidth  = aspect * nearHeight;
    farWidth   = aspect * farHeight;

    vec3 nc = pos + nearPlane * forward;
    vec3 fc = pos + farPlane * forward;

    vec3 Y = up;
    vec3 X = right;
    ntl    = nc + Y * nearHeight - X * nearWidth;
    ntr    = nc + Y * nearHeight + X * nearWidth;
    nbl    = nc - Y * nearHeight - X * nearWidth;
    nbr    = nc - Y * nearHeight + X * nearWidth;
    ftl    = fc + Y * farHeight - X * farWidth;
    ftr    = fc + Y * farHeight + X * farWidth;
    fbl    = fc - Y * farHeight - X * farWidth;
    fbr    = fc - Y * farHeight + X * farWidth;

    SetPlane( 0, ntl, nbl, fbl ); // left
    SetPlane( 1, nbr, ntr, fbr ); // right
    SetPlane( 2, ntr, ntl, ftl ); // top
    SetPlane( 3, nbl, nbr, fbr ); // bottom
    SetPlane( 4, ntl, ntr, nbr ); // near
    SetPlane( 5, ftr, ftl, fbl ); // far

#if !USING( SHIP_BUILD )
    corners[0] = ntl;
    corners[1] = ntr;
    corners[2] = nbr;
    corners[3] = nbl;
    corners[4] = ftl;
    corners[5] = ftr;
    corners[6] = fbr;
    corners[7] = fbl;
#endif // #if !USING( SHIP_BUILD )
}

void Frustum::ExtractFromVPMatrix( const mat4& VP )
{
    // left plane
    // 4th col + 1st col
    planes[0].x = VP[0][3] + VP[0][0];
    planes[0].y = VP[1][3] + VP[1][0];
    planes[0].z = VP[2][3] + VP[2][0];
    planes[0].w = VP[3][3] + VP[3][0];

    // right plane
    // 4th col - 1st col
    planes[1].x = VP[0][3] - VP[0][0];
    planes[1].y = VP[1][3] - VP[1][0];
    planes[1].z = VP[2][3] - VP[2][0];
    planes[1].w = VP[3][3] - VP[3][0];

    // top plane
    // 4th col - 2nd col
    planes[2].x = VP[0][3] - VP[0][1];
    planes[2].y = VP[1][3] - VP[1][1];
    planes[2].z = VP[2][3] - VP[2][1];
    planes[2].w = VP[3][3] - VP[3][1];

    // bottom plane
    // 4th col + 2nd col
    planes[3].x = VP[0][3] + VP[0][1];
    planes[3].y = VP[1][3] + VP[1][1];
    planes[3].z = VP[2][3] + VP[2][1];
    planes[3].w = VP[3][3] + VP[3][1];

    // near plane
    planes[4].x = VP[0][2];
    planes[4].y = VP[1][2];
    planes[4].z = VP[2][2];
    planes[4].w = VP[3][2];

    // far plane
    // 4th col - 3rd col
    planes[5].x = VP[0][3] - VP[0][2];
    planes[5].y = VP[1][3] - VP[1][2];
    planes[5].z = VP[2][3] - VP[2][2];
    planes[5].w = VP[3][3] - VP[3][2];

    // planes arent normalized yet
    for ( int i = 0; i < 6; ++i )
    {
        planes[i] /= Length( vec3( planes[i] ) );
    }

#if !USING( SHIP_BUILD )
    corners[0] = vec3( -1, -1, 0 );
    corners[1] = vec3( 1, -1, 0 );
    corners[2] = vec3( 1, 1, 0 );
    corners[3] = vec3( -1, 1, 0 );
    corners[4] = vec3( -1, -1, 1 );
    corners[5] = vec3( 1, -1, 1 );
    corners[6] = vec3( 1, 1, 1 );
    corners[7] = vec3( -1, 1, 1 );

    mat4 invVP = Inverse( VP );
    for ( int i = 0; i < 8; ++i )
    {
        vec4 p     = invVP * vec4( corners[i], 1 );
        corners[i] = vec3( p ) / p.w;
    }
#endif // #if !USING( SHIP_BUILD )
}

bool Frustum::BoxInFrustum( const AABB& aabb ) const
{
    for ( i32 i = 0; i < 6; ++i )
    {
        if ( !SameSide( aabb.P( vec3( planes[i] ) ), planes[i] ) )
        {
            return false;
        }
    }
    return true;
}

bool Frustum::SameSide( const vec3& point, const vec4& plane ) const
{
    return ( point.x * plane.x + point.y * plane.y + point.z * plane.z + plane.w ) >= 0;
}

void Frustum::SetPlane( i32 planeIndex, const vec3& p1, const vec3& p2, const vec3& p3 )
{
    vec3 p12 = p2 - p1;
    vec3 p13 = p3 - p1;

    vec3 n             = Normalize( Cross( p12, p13 ) );
    f32 d              = Dot( n, p1 );
    planes[planeIndex] = vec4( n, -d );
}

} // namespace PG
