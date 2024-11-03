#ifndef __CULL_DATA_H__
#define __CULL_DATA_H__

#include "c_shared/defines.h"

BEGIN_GPU_DATA_NAMESPACE()

struct AABB
{
    vec3 min;
    vec3 max;
};

struct CullData
{
    AABB aabb;
    uint modelIndex;
    uint numMeshlets;
};

struct MeshletDrawCommand
{
    uint groupCountX;
    uint groupCountY;
    uint groupCountZ;
    uint modelIndex;
    uint meshIndex;
    uint numMeshlets;
};

END_GPU_DATA_NAMESPACE()

#endif // __CULL_DATA_H__