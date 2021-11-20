#include "core/frustum.hpp"

namespace PG
{

bool Frustum::BoxInFrustum( const AABB& aabb ) const
{
    for ( int i = 0; i < 6; ++i )
    {
        if ( !SameSide( aabb.P( glm::vec3( planes[i] ) ), planes[i] ) )
        {
            return false;
        }
    }
    return true;
}


void Frustum::Update( float fov,
                      float nearPlane,
                      float farPlane,
                      float aspect,
                      const glm::vec3& pos,
                      const glm::vec3& forward,
                      const glm::vec3& up,
                      const glm::vec3& right )
{
    glm::vec3 ntl, ntr, nbl, nbr, ftl, ftr, fbr, fbl;
    float nearHeight, nearWidth, farHeight, farWidth;
    float angle = 0.5f * fov;

    nearHeight = nearPlane * tanf( angle );
    farHeight  = farPlane * tanf( angle );
    nearWidth  = aspect * nearHeight;
    farWidth   = aspect * farHeight;

    glm::vec3 nc = pos + nearPlane * forward;
    glm::vec3 fc = pos + farPlane * forward;

    glm::vec3 Y = up;
    glm::vec3 X = right;
    ntl         = nc + Y * nearHeight - X * nearWidth;
    ntr         = nc + Y * nearHeight + X * nearWidth;
    nbl         = nc - Y * nearHeight - X * nearWidth;
    nbr         = nc - Y * nearHeight + X * nearWidth;
    ftl         = fc + Y * farHeight - X * farWidth;
    ftr         = fc + Y * farHeight + X * farWidth;
    fbl         = fc - Y * farHeight - X * farWidth;
    fbr         = fc - Y * farHeight + X * farWidth;

    SetPlane( 0, ntl, ntr, nbr ); // near
    SetPlane( 1, ftr, ftl, fbl ); // far
    SetPlane( 2, ntl, nbl, fbl ); // left
    SetPlane( 3, nbr, ntr, fbr ); // right
    SetPlane( 4, ntr, ntl, ftl ); // top
    SetPlane( 5, nbl, nbr, fbr ); // bottom
    corners[0] = ntl;
    corners[1] = ntr;
    corners[2] = nbl;
    corners[3] = nbr;
    corners[4] = ftl;
    corners[5] = ftr;
    corners[6] = fbl;
    corners[7] = fbr;
}


bool Frustum::SameSide( const glm::vec3& point, const glm::vec4& plane ) const
{
    return ( point.x * plane.x + point.y * plane.y + point.z * plane.z + plane.w ) >= 0;
}


void Frustum::SetPlane( int planeIndex, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3 )
{
    glm::vec3 p12 = p2 - p1;
    glm::vec3 p13 = p3 - p1;

    glm::vec3 n = glm::normalize( glm::cross( p12, p13 ) );
    float d     = glm::dot( n, -p1 );
    planes[planeIndex] = glm::vec4( n, d );
}


} // namespace PG
