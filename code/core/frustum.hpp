#pragma once

#include "core/bounding_box.hpp"
#include "shared/platform_defines.hpp"

namespace PG
{

class Frustum
{
public:
    Frustum() = default;

    void Update( f32 vFov, f32 np, f32 fp, f32 aspect, const vec3& pos, const vec3& forward, const vec3& up, const vec3& right );
    void ExtractFromVPMatrix( const mat4& VP );

    bool BoxInFrustum( const AABB& aabb ) const;
    bool SameSide( const vec3& point, const vec4& plane ) const;

    vec4 planes[6];

#if !USING( SHIP_BUILD )
    // just kept around in non-ship builds for frustum debug drawing
    vec3 corners[8];
#endif // #if !USING( SHIP_BUILD )

private:
    void SetPlane( i32 planeIndex, const vec3& p1, const vec3& p2, const vec3& p3 );
};

} // namespace PG
