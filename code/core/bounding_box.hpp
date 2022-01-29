#pragma once

#include "shared/math.hpp"

namespace PG
{

class AABB
{
public:
    AABB();
    AABB( const glm::vec3& _min, const glm::vec3& _max );
    ~AABB() = default;

    glm::vec3 Center() const;
    void Points( glm::vec3* data ) const;
    int LongestDimension() const;
    float SurfaceArea() const;
    glm::mat4 ModelMatrix() const;
    glm::vec3 P( const glm::vec3& planeNormal ) const;
    glm::vec3 N( const glm::vec3& planeNormal ) const;
    glm::vec3 Offset( const glm::vec3& p ) const;

    void MoveCenterTo( const glm::vec3& point );
    void Encompass( const AABB& aabb );
    void Encompass( const AABB& aabb, const glm::mat4& transform );
    void Encompass( glm::vec3 point );
    void Encompass( glm::vec3* points, int numPoints );


    glm::vec3 min;
    glm::vec3 max;
};

} // namespace PG
