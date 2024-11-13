#ifndef __SCENE_GLOBALS_H__
#define __SCENE_GLOBALS_H__

#include "c_shared/defines.h"

BEGIN_GPU_DATA_NAMESPACE()

struct SceneGlobals
{
    mat4 V;
    mat4 P;
    mat4 VP;
    mat4 invVP;
    vec4 cameraPos; // w is 1, for convenience
    vec4 cameraExposureAndSkyTint; // x = exposure, yzw = sky tint

    vec4 frustumPlanes[6];
    vec4 cullingCameraPos;
    
    uint packedCullingDvarBools;
    uint modelMatriciesBufferIndex;
    uint normalMatriciesBufferIndex;
    uint r_tonemap;

    uint64_t lightBuffer;
    uvec4 numLights; // x = numLights, y,z,w = padding
    vec4 ambientColor;

    // debug vals
    uint r_geometryViz;
    uint r_materialViz;
    uint r_lightingViz;
    uint r_postProcessing;
    
    vec4 debug_wireframeData; // x,y,z = color, w = thickness

    uint debug_PackedDvarBools;
    int debugInt; // r_debugInt
    uint debugUint; // r_debugUint
    float debugFloat; // r_debugFloat
};

struct PerObjectData
{
    uint bindlessRangeStart;
    uint modelIdx;
    uint materialIdx;
};

struct NonIndirectPerObjectData
{
    uint bindlessRangeStart;
    uint modelIdx;
    uint materialIdx;
    uint numMeshlets;
};

END_GPU_DATA_NAMESPACE()

#endif // #ifndef __SCENE_GLOBALS_H__