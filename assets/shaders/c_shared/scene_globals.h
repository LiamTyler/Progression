#ifndef __SCENE_GLOBALS_H__
#define __SCENE_GLOBALS_H__

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
    
    UINT modelMatriciesIdx;
    UINT normalMatriciesIdx;
    UINT r_tonemap;
    UINT _pad;

    // debug vals
    UINT r_geometryViz;
    UINT r_materialViz;
    UINT r_lightingViz;
    UINT r_postProcessing;
    
    VEC4 debug_wireframeData; // x,y,z = color, w = thickness

    UINT debug_PackedDvarBools;
    int debugInt; // r_debugInt
    UINT debugUint; // r_debugUint
    float debugFloat; // r_debugFloat
};

struct PerObjectData
{
    UINT bindlessRangeStart;
    UINT objectIdx;
    UINT materialIdx;
};

#ifndef PG_SHADER_CODE
} // namespace GpuData
#endif // #ifndef PG_SHADER_CODE

#endif // #ifndef __SCENE_GLOBALS_H__