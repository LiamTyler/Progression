#ifndef __LIGHTS_GLSL__
#define __LIGHTS_GLSL__

#include "c_shared/structs.h"
#include "lib/packing.glsl"

struct PointLight
{
    vec3 color;
    float radius;
    vec3 position;
};

struct SpotLight
{
    vec3 color;
    float radius;
    vec3 position;
    float innerCutoffAngle;
    vec3 direction;
    float outerCutoffAngle;
};

struct DirectionalLight
{
    vec3 color;
    vec3 direction;
};


uint UnpackLightType( const PackedLight packedL )
{
    return uint( packedL.colorAndType.w );
}

PointLight UnpackPointLight( const PackedLight packedL )
{
    PointLight l;
    l.color = packedL.colorAndType.rgb;
    l.radius = packedL.positionAndRadius.w;
    l.position = packedL.positionAndRadius.xyz;

    return l;
}

SpotLight UnpackSpotLight( const PackedLight packedL )
{
    SpotLight l;
    l.color = packedL.colorAndType.rgb;
    l.radius = packedL.positionAndRadius.w;
    l.position = packedL.positionAndRadius.xyz;
    l.direction = packedL.directionAndSpotAngles.xyz;
    vec2 spotAngles = unpackHalf2x16( floatBitsToUint( packedL.directionAndSpotAngles.w ) );
    l.innerCutoffAngle = spotAngles.x;
    l.outerCutoffAngle = spotAngles.y;

    return l;
}

DirectionalLight UnpackDirectionalLight( const PackedLight packedL )
{
    DirectionalLight l;
    l.color = packedL.colorAndType.rgb;
    l.direction = packedL.directionAndSpotAngles.xyz;

    return l;
}


#endif // #ifndef __LIGHTS_GLSL__