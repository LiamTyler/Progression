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

#define MESH_BUFFER_MESHLETS 0
#define MESH_BUFFER_TRIS 1
#define MESH_BUFFER_POSITIONS 2
#define MESH_BUFFER_NORMALS 3
#define MESH_BUFFER_UVS 4
#define MESH_BUFFER_TANGENTS 5
#define MESH_NUM_BUFFERS 6

END_GPU_DATA_NAMESPACE()

#endif // #ifndef __MODEL_H__