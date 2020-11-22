#pragma once

#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"

namespace PG
{

struct Transform
{
    Transform() = default;

    glm::mat4 GetModelMatrix() const;

    glm::vec3 position = glm::vec3( 0 );
    glm::vec3 rotation = glm::vec3( 0 );
    glm::vec3 scale    = glm::vec3( 1 );
};

} // namespace PG
