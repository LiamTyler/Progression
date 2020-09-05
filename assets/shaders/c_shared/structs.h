#include "c_shared/defines.h"

struct SceneConstantBufferData
{
    MAT4 V;
    MAT4 P;
    MAT4 VP;
    MAT4 invVP;
    VEC4 cameraPos;
};

struct ObjectConstantBufferData
{
    MAT4 M;
    MAT4 N;
};

struct AnimatedObjectConstantBufferData
{
    MAT4 M;
    MAT4 N;
    UINT boneTransformIdx;
};

struct AnimatedShadowPerObjectData
{
    MAT4 MVP;
    UINT boneTransformIdx;
};

struct MaterialConstantBufferData
{
    VEC4 Kd;
    VEC4 Ks;
    UINT diffuseTexIndex;
    UINT normalMapIndex;
};

struct SSAOShaderData
{
    MAT4 V;
    MAT4 P;
};