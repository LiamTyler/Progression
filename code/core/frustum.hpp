#pragma once

#include "core/bounding_box.hpp"

namespace PG
{

class Frustum
{
public:
    Frustum() = default;

    void Update( f32 fov, f32 np, f32 fp, f32 aspect, const vec3& pos, const vec3& forward, const vec3& up, const vec3& right );

    bool BoxInFrustum( const AABB& aabb ) const;
    bool SameSide( const vec3& point, const vec4& plane ) const;

    vec4 planes[6];
    vec3 corners[8];

private:
    void SetPlane( i32 planeIndex, const vec3& p1, const vec3& p2, const vec3& p3 );
};

} // namespace PG
