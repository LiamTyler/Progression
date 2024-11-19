#include "ecs/components/transform.hpp"

namespace PG
{

Transform::Transform( vec3 inPosition, vec3 inRotation, vec3 inScale ) : position( inPosition ), rotation( inRotation ), scale( inScale ) {}

mat4 ModelMatrix( const vec3& scale, const vec3& rotation, const vec3& position )
{
    f32 sx = sin( rotation.x );
    f32 cx = cos( rotation.x );
    f32 sy = sin( rotation.y );
    f32 cy = cos( rotation.y );
    f32 sz = sin( rotation.z );
    f32 cz = cos( rotation.z );
    // clang-format off
    mat4 M(
        scale.x * (cz * cy - sx * sy * sz), scale.x * (sz * cy + cz * sx * sy), scale.x * (-cx * sy), 0,
        scale.y * (-sz * cx),               scale.y * (cx * cz),                scale.y * (sx),       0,
        scale.z * (cz * sy + sz * sx * cy), scale.z * (sy * sz - cy * cz * sx), scale.z * (cx * cy),  0,
        position.x, position.y, position.z, 1
    );
    // clang-format on
    return M;
}

mat4 Transform::Matrix() const { return ModelMatrix( scale, rotation, position ); }

mat4 Transform::PackedMatrixAndScale() const
{
    f32 sx = sin( rotation.x );
    f32 cx = cos( rotation.x );
    f32 sy = sin( rotation.y );
    f32 cy = cos( rotation.y );
    f32 sz = sin( rotation.z );
    f32 cz = cos( rotation.z );
    // clang-format off
    mat4 M(
        scale.x * (cz * cy - sx * sy * sz), scale.x * (sz * cy + cz * sx * sy), scale.x * (-cx * sy), 1.0f / (scale.x * scale.x),
        scale.y * (-sz * cx),               scale.y * (cx * cz),                scale.y * (sx),       1.0f / (scale.y * scale.y),
        scale.z * (cz * sy + sz * sx * cy), scale.z * (sy * sz - cy * cz * sx), scale.z * (cx * cy),  1.0f / (scale.z * scale.z),
        position.x, position.y, position.z, 1
    );
    // clang-format on
    return M;
}

vec3 Transform::TransformPoint( vec3 p ) const { return vec3( Matrix() * vec4( p, 1 ) ); }

vec3 Transform::TransformVector( vec3 v ) const { return vec3( Matrix() * vec4( v, 0 ) ); }

} // namespace PG
