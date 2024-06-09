#pragma once

#include "shared/math_vec.hpp"

namespace PT
{

vec2 UniformSampleDisk( f32 u1, f32 u2, f32 radius = 1 );

vec2 ConcentricSampleDisk( f32 u1, f32 u2, f32 radius = 1 );

vec3 UniformSampleHemisphere( f32 u1, f32 u2 );

vec3 CosineSampleHemisphere( f32 u1, f32 u2 );

vec3 UniformSampleSphere( f32 u1, f32 u2 );

vec2 UniformSampleTriangle( f32 u1, f32 u2 );

} // namespace PT
