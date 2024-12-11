#ifndef __MODEL_H__
#define __MODEL_H__

#include "c_shared/defines.h"

BEGIN_GPU_DATA_NAMESPACE()

#define PACKED_TRIS 1
#define PACKED_VERTS 1
#define BITS_PER_NORMAL 20

// if this is edited, the model version number needs to be bumped in the converter
struct Meshlet
{
    uint vertexOffset;
    uint triangleOffset;
    uint8_t vertexCount;
    uint8_t triangleCount;
    uint8_t _pad0;
    uint8_t _pad1;
    uint _pad2;

#if PACKED_VERTS
    uvec3 quantizedMeshletOffset;
    uint _pad3;
#endif // #if PACKED_VERTS
};

// if this is edited, the model version number needs to be bumped in the converter
struct PackedMeshletCullData
{
    vec3 position;
    float radius;
    vec3 coneAxis;
    float coneCutoff;
    vec3 coneApex;
    float pad;
};

struct MeshletCullData
{
    vec3 position;
    float radius;
    vec3 coneAxis;
    float coneCutoff;
    vec3 coneApex;
};

#define MAX_VERTS_PER_MESHLET 64
#define MAX_TRIS_PER_MESHLET 124

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
