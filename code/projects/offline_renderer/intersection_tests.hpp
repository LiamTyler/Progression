#pragma once

#include "shared/math.hpp"

namespace PT
{

struct Material;
struct IntersectionData
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
    glm::vec3 tangent;
    glm::vec3 bitangent;
    glm::vec3 wo;
    Material* material;
    float t = FLT_MAX;

    glm::vec3 dpdu, dpdv;
    glm::vec2 du, dv;
};

namespace intersect
{
    bool RayPlaneIntersection( const glm::vec3& rayPos, const glm::vec3& rayDir, const glm::vec3& N, const glm::vec3& pointOnPlane, float& t );

    bool RaySphere( const glm::vec3& rayPos, const glm::vec3& rayDir, const glm::vec3& spherePos, float sphereRadius, float& t, float maxT = FLT_MAX );

    bool RayTriangle( const glm::vec3& rayPos, const glm::vec3& rayDir, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, float& t, float& u, float& v, float maxT = FLT_MAX );

    bool RayAABB( const glm::vec3& rayPos, const glm::vec3& invRayDir, const glm::vec3& aabbMin, const glm::vec3& aabbMax, float maxT = FLT_MAX );

    bool RayAABBFastest( const glm::vec3& rayPos, const glm::vec3& invRayDir, const int isDirNeg[3], const glm::vec3& aabbMin, const glm::vec3& aabbMax, float maxT = FLT_MAX );

} // namespace intersect 
} // namespace PT