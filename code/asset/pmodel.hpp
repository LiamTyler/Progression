#pragma once

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace PG
{

enum class PModelVersionNum : unsigned int
{
    BITANGENT_SIGNS      = 1,
    VERTEX_DATA_TOGETHER = 2,

    TOTAL,
    CURRENT_VERSION        = TOTAL - 1,
    LAST_SUPPORTED_VERSION = VERTEX_DATA_TOGETHER,
};

#define PMODEL_MAX_UVS_PER_VERT 8u           // AI_MAX_NUMBER_OF_TEXTURECOORDS
#define PMODEL_MAX_COLORS_PER_VERT 8u        // AI_MAX_NUMBER_OF_COLOR_SETS
#define PMODEL_MAX_BONE_WEIGHTS_PER_VERT 16u // Assimp's is INT_MAX

class PModel
{
public:
    struct Vertex
    {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec3 tangent;
        glm::vec3 bitangent;
        glm::vec2 uvs[PMODEL_MAX_UVS_PER_VERT];
        glm::vec4 colors[PMODEL_MAX_COLORS_PER_VERT];
        float boneWeights[PMODEL_MAX_BONE_WEIGHTS_PER_VERT];
        uint32_t boneIndices[PMODEL_MAX_BONE_WEIGHTS_PER_VERT];
        uint8_t numBones = 0;

        bool AddBone( uint32_t boneIdx, float weight );
    };

    struct Mesh
    {
        std::string name;
        std::string materialName;
        std::vector<uint32_t> indices;
        std::vector<Vertex> vertices;
        uint32_t numUVChannels;
        uint32_t numColorChannels;
        bool hasTangents;
        bool hasBoneWeights;
    };

    std::vector<Mesh> meshes;

    bool Load( const std::string& filename );

    bool Save( std::ofstream& out, uint32_t floatPrecision = 6, bool logProgress = true ) const;
    bool Save( const std::string& filename, uint32_t floatPrecision = 6, bool logProgress = true ) const;
};

std::vector<std::string> GetUsedMaterialsPModel( const std::string& filename );

} // namespace PG
