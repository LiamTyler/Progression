#pragma once

#include "shared/math_vec.hpp"

namespace PT
{

struct Material;
struct IntersectionData
{
    vec3 position;
    vec3 normal;
    vec2 texCoords;
    vec3 tangent;
    vec3 bitangent;
    vec3 wo;
    Material* material;
    f32 t = FLT_MAX;

    vec3 dpdu, dpdv;
    vec2 du, dv;
};

namespace intersect
{

bool RayPlaneIntersection( const vec3& rayPos, const vec3& rayDir, const vec3& N, const vec3& pointOnPlane, f32& t );

bool RaySphere( const vec3& rayPos, const vec3& rayDir, const vec3& spherePos, f32 sphereRadius, f32& t, f32 maxT = FLT_MAX );

bool RayTriangle(
    const vec3& rayPos, const vec3& rayDir, const vec3& v0, const vec3& v1, const vec3& v2, f32& t, f32& u, f32& v, f32 maxT = FLT_MAX );

bool RayAABB( const vec3& rayPos, const vec3& invRayDir, const vec3& aabbMin, const vec3& aabbMax, f32 maxT = FLT_MAX );

bool RayAABBFastest(
    const vec3& rayPos, const vec3& invRayDir, const i32 isDirNeg[3], const vec3& aabbMin, const vec3& aabbMax, f32 maxT = FLT_MAX );

} // namespace intersect
} // namespace PT
