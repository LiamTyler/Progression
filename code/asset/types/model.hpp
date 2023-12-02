#pragma once

#include "asset/types/base_asset.hpp"
#include "core/feature_defines.hpp"
#if USING( GPU_DATA )
    #include "renderer/graphics_api/buffer.hpp"
#if USING( PG_RTX )
    #include "renderer/graphics_api/acceleration_structure.hpp"
#endif // #if USING( PG_RTX )
#endif     // #if USING( GPU_DATA )
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include <vector>

namespace PG
{

struct Material;

struct Mesh
{
    std::string name;
    uint32_t startIndex  = 0;
    uint32_t numIndices  = 0;
    uint32_t startVertex = 0;
    uint32_t numVertices = 0;
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
    void RecalculateNormals();
    void CreateBLAS();
    void UploadToGPU();
    void FreeCPU();
    void FreeGPU();

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec4> tangents; // xyz is the tangent, w is the bitangent sign
    std::vector<uint32_t> indices;

    std::vector<Mesh> meshes;
    std::vector<Material*> originalMaterials;

#if USING( GPU_DATA )
    uint32_t numVertices = 0;
    Gfx::Buffer vertexBuffer;
    Gfx::Buffer indexBuffer;
    size_t gpuPositionOffset;
    size_t gpuNormalOffset;
    size_t gpuTexCoordOffset;
    size_t gpuTangentOffset;
#if USING( PG_RTX )
    Gfx::AccelerationStructure blas;
#endif // #if USING( PG_RTX )
#endif     // #if USING( GPU_DATA )
};

} // namespace PG
