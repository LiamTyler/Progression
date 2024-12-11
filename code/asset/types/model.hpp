#pragma once

#include "asset/types/base_asset.hpp"
#include "c_shared/model.h"
#include "core/bounding_box.hpp"
#include "core/feature_defines.hpp"
#include "shared/math_vec.hpp"
#include <vector>

#if USING( GPU_DATA )
#include "renderer/graphics_api/buffer.hpp"
#endif // #if USING( GPU_DATA )

namespace PG
{

struct Material;

struct Mesh
{
    enum
    {
        VERTEX_BUFFER    = 0,
        TRI_BUFFER       = 1,
        MESHLET_BUFFER   = 2,
        CULL_DATA_BUFFER = 3,

        BUFFER_COUNT
    };

#if USING( ASSET_NAMES )
    std::string name;
#endif // #if USING( ASSET_NAMES )
    Material* material;

#if USING( GPU_DATA )
    Gfx::Buffer* buffers;
    u32 numVertices;
    u32 numMeshlets;
    bool hasTexCoords;
    bool hasTangents;
    u32 bindlessBuffersSlot;
#else
    std::vector<GpuData::Meshlet> meshlets;
    std::vector<GpuData::PackedMeshletCullData> meshletCullDatas;
#if PACKED_VERTS
    std::vector<u16vec3> packedPositions;
    std::vector<u32> packedNormals;
    std::vector<u32> packedTangents;
#else  // #if PACKED_VERTS
    std::vector<vec3> packedPositions;
    std::vector<vec3> packedNormals;
    std::vector<vec4> packedTangents; // xyz is the tangent, w is the bitangent sign
#endif // #else // #if PACKED_VERTS
    std::vector<vec2> texCoords;

#if PACKED_TRIS
    std::vector<u16> packedTris;
#else  // #if PACKED_TRIS
    std::vector<u8> packedTris;
#endif // #else // #if PACKED_TRIS
#endif // #if USING( GPU_DATA )
};

struct ModelCreateInfo : public BaseAssetCreateInfo
{
    std::string filename;
    bool flipTexCoordsVertically = false;
    bool recalculateNormals      = false;
    bool centerModel             = false;
};

std::string GetAbsPath_ModelFilename( const std::string& filename );

struct DequantizationInfo
{
    vec3 factor;
    vec3 globalMin;
};

struct Model : public BaseAsset
{
    bool Load( const BaseAssetCreateInfo* baseInfo ) override;
    bool FastfileLoad( Serializer* serializer ) override;
    bool FastfileSave( Serializer* serializer ) const override;
    void Free() override;
    void FreeCPU();
    void FreeGPU();

    std::vector<Mesh> meshes;
    std::vector<AABB> meshAABBs;
    DequantizationInfo positionDequantizationInfo;
};

} // namespace PG
