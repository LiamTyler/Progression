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

        BUFFER_COUNT = 4
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
    std::vector<vec3> positions;
    std::vector<vec3> normals;
    std::vector<vec2> texCoords;
    std::vector<vec4> tangents; // xyz is the tangent, w is the bitangent sign
    std::vector<u8> indices;
#endif // #if USING( GPU_DATA )
};

struct ModelCreateInfo : public BaseAssetCreateInfo
{
    std::string filename;
    bool flipTexCoordsVertically = false;
    bool recalculateNormals      = false;
};

std::string GetAbsPath_ModelFilename( const std::string& filename );

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
};

} // namespace PG
