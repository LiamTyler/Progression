#pragma once

#include "shared/math.hpp"

namespace PG
{

class AABB
{
public:
    AABB();
    AABB( const vec3& _min, const vec3& _max );
    ~AABB() = default;

    vec3 Center() const;
    void Points( vec3* data ) const;
    int LongestDimension() const;
    float SurfaceArea() const;
    mat4 ModelMatrix() const;
    vec3 P( const vec3& planeNormal ) const;
    vec3 N( const vec3& planeNormal ) const;
    vec3 Offset( const vec3& p ) const;

    void MoveCenterTo( const vec3& point );
    void Encompass( const AABB& aabb );
    void Encompass( const AABB& aabb, const mat4& transform );
    void Encompass( vec3 point );
    void Encompass( vec3* points, int numPoints );

    vec3 min;
    vec3 max;
};

} // namespace PG
