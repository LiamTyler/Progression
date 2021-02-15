#pragma once

#include "asset/types/base_asset.hpp"
#include "renderer/graphics_api/buffer.hpp"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
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
};

struct Model : public BaseAsset
{
    bool Load( const BaseAssetCreateInfo* baseInfo ) override;
    bool FastfileLoad( Serializer* serializer ) override;
    bool FastfileSave( Serializer* serializer ) const override;
    void Free() override;
    void RecalculateNormals();
    void UploadToGPU();
    void FreeCPU();
    void FreeGPU();

    std::vector< glm::vec3 > vertexPositions;
    std::vector< glm::vec3 > vertexNormals;
    std::vector< glm::vec2 > vertexTexCoords;
    std::vector< glm::vec3 > vertexTangents;
    std::vector< uint32_t > indices;

    std::vector< Mesh > meshes;
    std::vector< Material* > originalMaterials;

    Gfx::Buffer vertexBuffer;
    Gfx::Buffer indexBuffer;
    size_t gpuPositionOffset;
    size_t gpuNormalOffset;
    size_t gpuTexCoordOffset;
    size_t gpuTangentOffset;
};


// Need this during the converter, since the normal Model_Load actually tries to lookup the materials in the AssetManager,
// but the AssetManager is empty during the Converter. Just need the model data + material names
bool Model_Load_PGModel( Model* model, const ModelCreateInfo& createInfo, std::vector< std::string >& matNames );

bool Model_Load( Model* model, const ModelCreateInfo& createInfo );


bool Fastfile_Model_Load( Model* model, Serializer* serializer );

bool Fastfile_Model_Save( const Model * const model, Serializer* serializer, const std::vector< std::string >& matNames );

} // namespace PG