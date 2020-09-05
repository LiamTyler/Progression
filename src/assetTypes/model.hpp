#pragma once

#include "assetTypes/base_asset.hpp"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include <vector>

class Serializer;

namespace Progression
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

struct OtherVertexData
{
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec3 tangent;
};

struct ModelCreateInfo
{
    std::string name;
    std::string filename;
};

struct Model : public Asset
{
    void RecalculateNormals();

    std::vector< glm::vec3 > vertexPositions;
    std::vector< OtherVertexData > otherVertexData;
    std::vector< uint32_t > indices;

    std::vector< Mesh > meshes;
    std::vector< Material* > originalMaterials;
};


// Need this during the converter, since the normal Model_Load actually tries to lookup the materials in the AssetManager,
// but the AssetManager is empty during the Converter. Just need the model data + material names
bool Model_Load_NoResolveMaterials( Model* model, const ModelCreateInfo& createInfo, std::vector< std::string >& matNames );

bool Model_Load( Model* model, const ModelCreateInfo& createInfo );


bool Fastfile_Model_Load( Model* model, Serializer* serializer );

bool Fastfile_Model_Save( const Model * const model, Serializer* serializer, const std::vector< std::string >& matNames );

} // namespace Progression