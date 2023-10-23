#ifndef __LIGHTS_GLSL__
#define __LIGHTS_GLSL__

#include "c_shared/structs.h"
#include "lib/packing.glsl"

struct PointLight
{
    vec3 color;
    float radius;
    vec3 position;
    float invRadiusSquared;
};

struct SpotLight
{
    vec3 color;
    float radius;
    float invRadiusSquared;
    vec3 position;
    float cosOuterAngle;
    vec3 direction;
    float invCosAngleDiff;
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
    vec2 radii = unpackHalf2x16( floatBitsToUint( packedL.positionAndRadius.w ) );
    l.radius = radii.x;
    l.invRadiusSquared = radii.y;
    l.position = packedL.positionAndRadius.xyz;

    return l;
}

SpotLight UnpackSpotLight( const PackedLight packedL )
{
    SpotLight l;
    l.color = packedL.colorAndType.rgb;
    vec2 radii = unpackHalf2x16( floatBitsToUint( packedL.positionAndRadius.w ) );
    l.radius = radii.x;
    l.invRadiusSquared = radii.y;
    l.position = packedL.positionAndRadius.xyz;
    l.direction = packedL.directionAndSpotAngles.xyz;
    vec2 spotAngles = unpackHalf2x16( floatBitsToUint( packedL.directionAndSpotAngles.w ) );
    l.cosOuterAngle = spotAngles.x;
    l.invCosAngleDiff = spotAngles.y;

    return l;
}

DirectionalLight UnpackDirectionalLight( const PackedLight packedL )
{
    DirectionalLight l;
    l.color = packedL.colorAndType.rgb;
    l.direction = packedL.directionAndSpotAngles.xyz;

    return l;
}

// Point + Spot attenutation copied from UE5
float LightDistanceAttenuation( vec3 lightPos, vec3 pixelPos, float invRadiusSquared )
{
    vec3 diff = lightPos - pixelPos;
    float d2 = dot( diff, diff );
    float ratio2 = d2 * invRadiusSquared;
    float radiusFalloff = max( 0.0f, 1.0f - ratio2 * ratio2 );
    radiusFalloff *= radiusFalloff;
    float distanceFalloff = 1.0f / (1.0f * d2);

    return distanceFalloff * radiusFalloff;
}

float PointLightAttenuation( vec3 pos, PointLight l )
{
    return LightDistanceAttenuation( l.position, pos, l.invRadiusSquared );
}

float SpotLightAttenuation( vec3 pos, SpotLight l )
{
    float distanceAttenuation = LightDistanceAttenuation( l.position, pos, l.invRadiusSquared );

    vec3 L = normalize( l.position - pos );
    float spotAttenuation = clamp( (dot( -l.direction, L ) - l.cosOuterAngle) * l.invCosAngleDiff, 0.0f, 1.0f );
    spotAttenuation *= spotAttenuation;

    return distanceAttenuation * spotAttenuation;
}


#endif // #ifndef __LIGHTS_GLSL__