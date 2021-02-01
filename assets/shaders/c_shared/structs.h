#include "c_shared/defines.h"

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
    float metalness;
    float roughness;
    UINT albedoMapIndex;
    UINT metalnessMapIndex;
    UINT roughnessMapIndex;
};

#define PG_SCENE_GLOBALS_BUFFER_SET 0
#define PG_BINDLESS_TEXTURE_SET     1