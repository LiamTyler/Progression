#pragma once

#include "asset/types/base_asset.hpp"
#include "core/feature_defines.hpp"
#include "shared/math_vec.hpp"
#include <vector>

#if USING( GPU_DATA )
#include "renderer/graphics_api/buffer.hpp"
#if USING( PG_RTX )
#include "renderer/graphics_api/acceleration_structure.hpp"
#endif // #if USING( PG_RTX )
#endif // #if USING( GPU_DATA )

namespace PG
{

struct Material;

struct Meshlet
{
    uint32_t vertexOffset;
    uint32_t indexOffset;
    uint32_t _pad1;
    uint16_t _pad2;
    uint8_t vertexCount;
    uint8_t triangleCount;
};

struct Mesh
{
    std::string name;
    Material* material;
    std::vector<Meshlet> meshlets;
    std::vector<vec3> positions;
    std::vector<vec3> normals;
    std::vector<vec2> texCoords;
    std::vector<vec4> tangents; // xyz is the tangent, w is the bitangent sign
    std::vector<uint8_t> indices;

#if USING( GPU_DATA )
    Gfx::Buffer vertexBuffer;
    Gfx::Buffer meshletBuffer;
    size_t indexOffset;
    uint32_t numVertices;
    bool hasTexCoords;
    bool hasTangents;
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
    void CreateBLAS();
    void UploadToGPU();
    void FreeCPU();
    void FreeGPU();

    std::vector<Mesh> meshes;
};

} // namespace PG
