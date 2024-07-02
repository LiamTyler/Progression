#ifndef __MODEL_H__
#define __MODEL_H__

#include "c_shared/defines.h"

BEGIN_GPU_DATA_NAMESPACE()

struct Meshlet
{
    uint vertexOffset;
    uint triOffset;
    uint vertexCount;
    uint triangleCount;
};

struct PackedMeshletCullData
{
    uint positionXY;
    uint positionZAndRadius;
    uint coneAxisAndCutoff;
    uint _padTo16Bytes;
};

#define MESH_BUFFER_MESHLETS 0
#define MESH_BUFFER_MESHLET_CULL_DATA 1
#define MESH_BUFFER_TRIS 2
#define MESH_BUFFER_POSITIONS 3
#define MESH_BUFFER_NORMALS 4
#define MESH_BUFFER_UVS 5
#define MESH_BUFFER_TANGENTS 6
#define MESH_NUM_BUFFERS 7

END_GPU_DATA_NAMESPACE()

#endif // #ifndef __MODEL_H__