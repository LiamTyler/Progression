#pragma once

#include "shared/math.hpp"

namespace PG
{

struct Transform
{
    Transform() = default;
    Transform( glm::vec3 inPosition, glm::vec3 inRotation, glm::vec3 inScale );

    glm::mat4 Matrix() const;
    glm::vec3 TransformPoint( glm::vec3 p ) const;
    glm::vec3 TransformVector( glm::vec3 v ) const;
    Ray operator*( const Ray& ray ) const;

    glm::vec3 position = glm::vec3( 0 );
    glm::vec3 rotation = glm::vec3( 0 );
    glm::vec3 scale    = glm::vec3( 1 );
};

} // namespace PG
