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

#define PG_SCENE_GLOBALS_BUFFER_SET 0