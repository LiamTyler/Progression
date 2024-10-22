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
    vec4 _pad; // just for 64 byte alignment
};

END_GPU_DATA_NAMESPACE()

#endif // #ifndef __LIGHTS_H__