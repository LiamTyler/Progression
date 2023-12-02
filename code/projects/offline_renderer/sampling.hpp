#pragma once

#include "shared/math.hpp"

namespace PT
{

glm::vec2 UniformSampleDisk( float u1, float u2, float radius = 1 );

glm::vec2 ConcentricSampleDisk( float u1, float u2, float radius = 1 );

glm::vec3 UniformSampleHemisphere( float u1, float u2 );

glm::vec3 CosineSampleHemisphere( float u1, float u2 );

glm::vec3 UniformSampleSphere( float u1, float u2 );

glm::vec2 UniformSampleTriangle( float u1, float u2 );

} // namespace PT
