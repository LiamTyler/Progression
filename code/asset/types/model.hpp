#pragma once

#include "asset/types/base_asset.hpp"
#include "core/feature_defines.hpp"
#include "shared/math_vec.hpp"
#include <vector>

#if USING( GPU_DATA )
#include "c_shared/model.h"
#include "renderer/graphics_api/buffer.hpp"
#endif // #if USING( GPU_DATA )

namespace PG
{

struct Meshlet
{
    uint32_t vertexOffset;
    uint32_t triOffset;
    uint32_t vertexCount;
    uint32_t triangleCount;
};

struct Material;

struct Mesh
{
#if USING( ASSET_NAMES )
    std::string name;
#endif // #if USING( ASSET_NAMES )
    Material* material;
    std::vector<Meshlet> meshlets;
    std::vector<vec3> positions;
    std::vector<vec3> normals;
    std::vector<vec2> texCoords;
    std::vector<vec4> tangents; // xyz is the tangent, w is the bitangent sign
    std::vector<uint8_t> indices;

#if USING( GPU_DATA )
    Gfx::Buffer vertexBuffer;
    Gfx::Buffer triBuffer;
    Gfx::Buffer meshletBuffer;
    uint32_t numVertices;
    uint32_t numMeshlets;
    bool hasTexCoords;
    bool hasTangents;
    uint32_t bindlessBuffersSlot;
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
    void UploadToGPU();
    void FreeCPU();
    void FreeGPU();

    std::vector<Mesh> meshes;
};

} // namespace PG
