#pragma once

#include "core/math.hpp"

namespace PG
{

class AABB
{
public:
    AABB( const glm::vec3& min_ = glm::vec3( 0 ), const glm::vec3& max_ = glm::vec3( 0 ) );
    ~AABB() = default;

    glm::vec3 GetCenter() const;
    void GetPoints( glm::vec3* data ) const;
    void SetCenter( const glm::vec3& point );
    void Encompass( const AABB& aabb, const glm::mat4& transform );
    void Encompass( glm::vec3* points, int numPoints );
    glm::mat4 GetModelMatrix() const;
    glm::vec3 GetP( const glm::vec3& planeNormal ) const;
    glm::vec3 GetN( const glm::vec3& planeNormal ) const;

    glm::vec3 min;
    glm::vec3 max;
    glm::vec3 extent;
};

} // namespace PG
