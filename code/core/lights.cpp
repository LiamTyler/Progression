#include "lights.hpp"
#include "shared/float_conversions.hpp"

using namespace GpuData;
using namespace glm;

namespace PG
{

#if USING( GAME )
// NOTE! Keep the following Pack* functions in sync with lights.glsl
GpuData::PackedLight PackPointLight( const PointLight& light )
{
    PackedLight p;
    p.colorAndType = vec4( light.intensity * light.color, LIGHT_TYPE_POINT );
    p.positionAndRadius = vec4( light.position, light.radius );

    return p;
}


GpuData::PackedLight PackSpotLight( const SpotLight& light )
{
    PackedLight p;
    p.colorAndType = vec4( light.intensity * light.color, LIGHT_TYPE_SPOT );
    p.positionAndRadius = vec4( light.position, light.radius );
    uint32_t packedAngles = Float32ToFloat16( light.innerCutoffAngle, light.outerCutoffAngle );
    p.directionAndSpotAngles = vec4( light.direction, packedAngles );

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
