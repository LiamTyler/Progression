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
    VEC4 cameraPos;
};

struct PerObjectData
{
    MAT4 M;
    MAT4 N;
};

struct MaterialData
{
    VEC4 albedoTint;
    float metalnessTint;
    UINT albedoMetalnessMapIndex;
    UINT irradianceMapIndex;
    float pad2;
};

struct PointLight
{
    VEC4 positionAndRadius;
    VEC4 color;
};

struct SkyboxData
{
    VEC3 tint;
    float scale;
	UINT hasTexture;
    UINT debug;
    UVEC2 _pad1;
};

#ifndef PG_SHADER_CODE
} // namespace GpuData
#endif // #ifndef PG_SHADER_CODE

#define PG_SCENE_GLOBALS_BUFFER_SET 0
#define PG_BINDLESS_TEXTURE_SET     1
#define PG_LIGHTS_SET               2
    #define PG_POINT_LIGHTS_BIND_INDEX  0
    #define PG_SPOT_LIGHTS_BIND_INDEX   1

#endif // #ifndef _STRUCTS_H_