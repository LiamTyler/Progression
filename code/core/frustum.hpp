#pragma once

#include "core/bounding_box.hpp"

namespace PG
{

class Frustum
{
public:
    Frustum() = default;

    void Update( float fov, float np, float fp, float aspect, const glm::vec3& pos, const glm::vec3& forward, const glm::vec3& up,
        const glm::vec3& right );

    bool BoxInFrustum( const AABB& aabb ) const;
    bool SameSide( const glm::vec3& point, const glm::vec4& plane ) const;

    glm::vec4 planes[6];
    glm::vec3 corners[8];

private:
    void SetPlane( int planeIndex, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3 );
};

} // namespace PG
