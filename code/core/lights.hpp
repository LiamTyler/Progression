#pragma once

#include "glm/trigonometric.hpp"
#include "glm/vec3.hpp"

namespace PG
{

struct Light
{
    glm::vec3 color = glm::vec4( 1 );
    float intensity = 1;
};

struct DirectionalLight : public Light
{
    glm::vec3 direction = glm::vec4( 0, 0, -1, 0 );
};

struct PointLight : public Light
{
    glm::vec3 position = glm::vec3( 0, 0, 0 );
    float radius       = 10;
};

struct SpotLight : public Light
{
    glm::vec3 position  = glm::vec3( 0, 0, 0 );
    float radius        = 10;
    glm::vec3 direction = glm::vec3( 0, 0, -1 );
    float cutoff        = glm::radians( 20.0f );
};

} // namespace PG
