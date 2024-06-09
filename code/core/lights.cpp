#include "lights.hpp"
#include "shared/float_conversions.hpp"

namespace PG
{

static f32 PackRadius( f32 radius )
{
    f32 inv    = 1.0f / radius;
    u32 packed = Float32ToFloat16( radius, inv * inv );
    return *reinterpret_cast<f32*>( &packed );
}

/*
#if USING( GAME )
// NOTE! Keep the following Pack* functions in sync with lights.glsl
GpuData::PackedLight PackPointLight( const PointLight& light )
{
    PackedLight p;
    p.colorAndType      = vec4( light.intensity * light.color, LIGHT_TYPE_POINT );
    p.positionAndRadius = vec4( light.position, PackRadius( light.radius ) );

    return p;
}

GpuData::PackedLight PackSpotLight( const SpotLight& light )
{
    PackedLight p;
    p.colorAndType           = vec4( light.intensity * light.color, LIGHT_TYPE_SPOT );
    p.positionAndRadius      = vec4( light.position, PackRadius( light.radius ) );
    f32 cosOuterAngle      = cos( light.outerAngle );
    f32 invCosAngleDiff    = 1.0f / ( cos( light.innerAngle ) - cosOuterAngle );
    u32 packedAngles    = Float32ToFloat16( cosOuterAngle, invCosAngleDiff );
    p.directionAndSpotAngles = vec4( light.direction, *reinterpret_cast<f32*>( &packedAngles ) );

    return p;
}

GpuData::PackedLight PackDirectionalLight( const DirectionalLight& light )
{
    PackedLight p;
    p.colorAndType           = vec4( light.intensity * light.color, LIGHT_TYPE_DIRECTIONAL );
    p.directionAndSpotAngles = vec4( light.direction, 0 );

    return p;
}
#endif // #if USING( GAME )
*/

} // namespace PG
