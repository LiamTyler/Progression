#pragma once

#include "shared/math_vec.hpp"

namespace PT
{

vec2 UniformSampleDisk( float u1, float u2, float radius = 1 );

vec2 ConcentricSampleDisk( float u1, float u2, float radius = 1 );

vec3 UniformSampleHemisphere( float u1, float u2 );

vec3 CosineSampleHemisphere( float u1, float u2 );

vec3 UniformSampleSphere( float u1, float u2 );

vec2 UniformSampleTriangle( float u1, float u2 );

} // namespace PT
