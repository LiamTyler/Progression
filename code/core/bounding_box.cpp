#include "core/bounding_box.hpp"


namespace PG
{

AABB::AABB( const glm::vec3& min_, const glm::vec3& max_ ) :
  min( min_ ), max( max_ ), extent( max_ - min_ )
{
}


glm::vec3 AABB::GetCenter() const
{
    return ( max + min ) / 2.0f;
}


void AABB::GetPoints( glm::vec3* data ) const
{
    data[0] = min + glm::vec3( 0 );
    data[1] = min + glm::vec3( extent.x, 0, 0 );
    data[2] = min + glm::vec3( extent.x, 0, extent.z );
    data[3] = min + glm::vec3( 0, 0, extent.z );
    data[4] = max - glm::vec3( 0 );
    data[5] = max - glm::vec3( extent.x, 0, 0 );
    data[6] = max - glm::vec3( extent.x, 0, extent.z );
    data[7] = max - glm::vec3( 0, 0, extent.z );
}


void AABB::SetCenter( const glm::vec3& point )
{
    min = point - extent / 2.0f;
    max = point + extent / 2.0f;
}


void AABB::Encompass( glm::vec3* points, int numPoints )
{
    for ( int i = 0; i < numPoints; ++i )
    {
        min = glm::min( min, points[i] );
        max = glm::max( max, points[i] );
    }
    extent = max - min;
}


void AABB::Encompass( const AABB& aabb, const glm::mat4& transform )
{
    glm::vec3 he        = aabb.extent / 2.0f;
    glm::vec3 points[8] = { -he,
                            glm::vec3( he.x, -he.y, -he.z ),
                            glm::vec3( -he.x, -he.y, he.z ),
                            glm::vec3( he.x, -he.y, he.z ),
                            he,
                            glm::vec3( he.x, he.y, -he.z ),
                            glm::vec3( -he.x, he.y, he.z ),
                            glm::vec3( -he.x, he.y, -he.z ) };

    min = max = glm::vec3( transform * glm::vec4( points[0], 1 ) );
    for ( int i = 1; i < 8; ++i )
    {
        glm::vec3 tmp = glm::vec3( transform * glm::vec4( points[i], 1 ) );
        min = glm::min( min, tmp );
        max = glm::max( max, tmp );
    }
    extent = max - min;
}


glm::mat4 AABB::GetModelMatrix() const
{
    glm::vec3 center = .5f * ( min + max );
    glm::vec3 scale  = 0.5f * extent;
    glm::mat4 model = glm::mat4(
        scale.x, 0, 0, 0,
        0, scale.y, 0, 0,
        0, 0, scale.z, 0,
        center.x, center.y, center.z, 1
    );
    return model;
}


glm::vec3 AABB::GetP( const glm::vec3& planeNormal ) const
{
    glm::vec3 P = min;
    if ( planeNormal.x >= 0 )
        P.x = max.x;
    if ( planeNormal.y >= 0 )
        P.y = max.y;
    if ( planeNormal.z >= 0 )
        P.z = max.z;

    return P;
}


glm::vec3 AABB::GetN( const glm::vec3& planeNormal ) const
{
    glm::vec3 N = max;
    if ( planeNormal.x >= 0 )
        N.x = min.x;
    if ( planeNormal.y >= 0 )
        N.y = min.y;
    if ( planeNormal.z >= 0 )
        N.z = min.z;

    return N;
}


} // namespace PG
