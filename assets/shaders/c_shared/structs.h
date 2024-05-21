#ifndef _STRUCTS_H_
#define _STRUCTS_H_

#include "c_shared/defines.h"

#ifndef PG_SHADER_CODE
namespace GpuData
{
#endif // #ifndef PG_SHADER_CODE

struct SceneGlobals
{
    MAT4 V;
    MAT4 P;
    MAT4 VP;
    MAT4 invVP;
    VEC4 cameraPos; // w is 1, for convenience
    VEC4 cameraExposureAndSkyTint; // x = exposure, yzw = sky tint
    UVEC4 lightCountAndPad3; // x = light count, yzw = pad
    
    UINT r_materialViz;
    UINT r_lightingViz;
    UINT r_postProcessing;
    UINT r_tonemap;
    
    int debugInt; // r_debugInt
    UINT debugUint; // r_debugUint
    float debugFloat; // r_debugFloat
    UINT debug3;
};

struct PerObjectData
{
    UINT matrixIndex;
};

struct Meshlet
{
    UINT vertexOffset;
    UINT triOffset;
    UINT vertexCount;
    UINT triangleCount;
};

struct MaterialData
{
    VEC3 albedoTint;
    float metalnessTint;
    
    float roughnessTint;
    UINT albedoMetalnessMapIndex;
    UINT normalRoughnessMapIndex;
    float pad;
    
    VEC3 emissiveTint;
    UINT emissiveMapIndex;
};

#define LIGHT_TYPE_POINT 0
#define LIGHT_TYPE_SPOT 1
#define LIGHT_TYPE_DIRECTIONAL 2

struct PackedLight
{
    VEC4 positionAndRadius; // xyz = world position, w = 2xfp16: radius and invRadiusSquared
    VEC4 colorAndType; // xyz = color, w = light type
    VEC4 directionAndSpotAngles; // xyz = direction, w = 2xfp16: cosOuterSpotAngle and invCosSpotAngleDiff
    VEC4 _pad; // just for 64 byte alignment
};

struct SkyboxData
{
    MAT4 VP;
    VEC3 tint;
    float scale;
    UINT hasTexture;
    UINT r_skyboxViz;
    float r_skyboxReflectionMipLevel;
    UINT _pad1;
};

#ifndef PG_SHADER_CODE
} // namespace GpuData
#endif // #ifndef PG_SHADER_CODE

#endif // #ifndef _STRUCTS_H_