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
    vec3 Extent() const;
    void Points( vec3* data ) const;
    i32 LongestDimension() const;
    f32 SurfaceArea() const;
    mat4 ModelMatrix() const;
    vec3 P( const vec3& planeNormal ) const;
    vec3 N( const vec3& planeNormal ) const;
    vec3 Offset( const vec3& p ) const;

    void Scale( vec3 v );
    void MoveCenterTo( const vec3& point );
    void Encompass( const AABB& aabb );
    void Encompass( vec3 point );
    void Encompass( vec3* points, i32 numPoints );

    AABB Transform( const mat4& transform ) const;

    vec3 min;
    vec3 max;
};

class AABB2D
{
public:
    AABB2D();
    AABB2D( const vec2& _min, const vec2& _max );
    ~AABB2D() = default;

    vec2 Center() const;
    vec2 Extent() const;
    i32 LongestDimension() const;
    f32 Area() const;

    void Encompass( const AABB2D& aabb );
    void Encompass( vec2 point );

    vec2 min;
    vec2 max;
};

} // namespace PG
