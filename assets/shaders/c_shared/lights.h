#ifndef __LIGHTS_H__
#define __LIGHTS_H__

#include "c_shared/defines.h"

BEGIN_GPU_DATA_NAMESPACE()

#define LIGHT_TYPE_DIRECTIONAL 0
#define LIGHT_TYPE_POINT 1
#define LIGHT_TYPE_SPOT 2

struct PackedLight
{
    vec4 positionAndRadius; // xyz = world position, w = 2xfp16: radius and invRadiusSquared
    vec4 colorAndType; // xyz = color, w = light type
    vec4 directionAndSpotAngles; // xyz = direction, w = 2xfp16: cosOuterSpotAngle and invCosSpotAngleDiff
    uvec4 shadowMapIdx; // x = shadowMapIdx (0 == no shadow map), y,z,w = pad
};

END_GPU_DATA_NAMESPACE()

#endif // #ifndef __LIGHTS_H__