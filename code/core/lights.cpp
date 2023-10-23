#include "lights.hpp"
#include "shared/float_conversions.hpp"

using namespace GpuData;
using namespace glm;

namespace PG
{

static float PackRadius( float radius )
{
    float inv = 1.0f / radius;
    uint32_t packed = Float32ToFloat16( radius, inv * inv );
    return *reinterpret_cast<float*>( &packed );
}

#if USING( GAME )
// NOTE! Keep the following Pack* functions in sync with lights.glsl
GpuData::PackedLight PackPointLight( const PointLight& light )
{
    PackedLight p;
    p.colorAndType = vec4( light.intensity * light.color, LIGHT_TYPE_POINT );
    p.positionAndRadius = vec4( light.position, PackRadius( light.radius ) );

    return p;
}


GpuData::PackedLight PackSpotLight( const SpotLight& light )
{
    PackedLight p;
    p.colorAndType = vec4( light.intensity * light.color, LIGHT_TYPE_SPOT );
    p.positionAndRadius = vec4( light.position, PackRadius( light.radius ) );
    float cosOuterAngle = cos( light.outerAngle );
    float invCosAngleDiff = 1.0f / (cos( light.innerAngle ) - cosOuterAngle);
    uint32_t packedAngles = Float32ToFloat16( cosOuterAngle, invCosAngleDiff );
    p.directionAndSpotAngles = vec4( light.direction, *reinterpret_cast<float*>( &packedAngles ) );

    return p;
}


GpuData::PackedLight PackDirectionalLight( const DirectionalLight& light )
{
    PackedLight p;
    p.colorAndType = vec4( light.intensity * light.color, LIGHT_TYPE_DIRECTIONAL );
    p.directionAndSpotAngles = vec4( light.direction, 0 );

    return p;
}
#endif // #if USING( GAME )

} // namespace PG
