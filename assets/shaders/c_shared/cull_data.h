#ifndef __CULL_DATA_H__
#define __CULL_DATA_H__

#include "c_shared/defines.h"

BEGIN_GPU_DATA_NAMESPACE()

struct AABB
{
    vec3 min;
    vec3 max;
};

struct MeshCullData
{
    AABB aabb;
    uint modelIndex;
    uint numMeshlets;
    uint bindlessRangeStart;
};

#define MAX_CULLING_COMPUTE_DIMENSION 65535
#define MESHLET_CULL_SHADER_WORKGROUP_SIZE 32
#define PREFIX_SHADER_WORKGROUP_SIZE 512
#define PREFIX_SHADER_ITEMS_PER_WORKGROUP (2 * PREFIX_SHADER_WORKGROUP_SIZE)

struct MeshletDrawCommand
{
    // VkDrawMeshTasksIndirectCommandEXT
    uint groupCountX;
    uint groupCountY;
    uint groupCountZ;

    // custom
    uint modelIndex; // is this needed?
    uint meshIndex;
    uint numMeshlets; // is this needed?
};


END_GPU_DATA_NAMESPACE()

#endif // __CULL_DATA_H__