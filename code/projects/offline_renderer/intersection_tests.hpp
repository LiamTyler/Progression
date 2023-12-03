#pragma once

#include "shared/math_vec.hpp"
#include <cfloat>

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
    float t = FLT_MAX;

    vec3 dpdu, dpdv;
    vec2 du, dv;
};

namespace intersect
{

bool RayPlaneIntersection( const vec3& rayPos, const vec3& rayDir, const vec3& N, const vec3& pointOnPlane, float& t );

bool RaySphere( const vec3& rayPos, const vec3& rayDir, const vec3& spherePos, float sphereRadius, float& t, float maxT = FLT_MAX );

bool RayTriangle( const vec3& rayPos, const vec3& rayDir, const vec3& v0, const vec3& v1, const vec3& v2, float& t, float& u, float& v,
    float maxT = FLT_MAX );

bool RayAABB( const vec3& rayPos, const vec3& invRayDir, const vec3& aabbMin, const vec3& aabbMax, float maxT = FLT_MAX );

bool RayAABBFastest(
    const vec3& rayPos, const vec3& invRayDir, const int isDirNeg[3], const vec3& aabbMin, const vec3& aabbMax, float maxT = FLT_MAX );

} // namespace intersect
} // namespace PT
