#pragma once

#include "shared/math_vec.hpp"
#include <fstream>
#include <string>
#include <vector>

namespace PG
{

enum class PModelVersionNum : u16
{
    BITANGENT_SIGNS      = 1,
    VERTEX_DATA_TOGETHER = 2,
    MODEL_AABB           = 3,

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
        vec3 pos;
        vec3 normal;
        vec3 tangent;
        vec3 bitangent;
        vec2 uvs[PMODEL_MAX_UVS_PER_VERT];
        vec4 colors[PMODEL_MAX_COLORS_PER_VERT];
        f32 boneWeights[PMODEL_MAX_BONE_WEIGHTS_PER_VERT];
        u32 boneIndices[PMODEL_MAX_BONE_WEIGHTS_PER_VERT];
        u8 numBones = 0;

        bool AddBone( u32 boneIdx, f32 weight );
    };

    struct Mesh
    {
        std::string name;
        std::string materialName;
        std::vector<u32> indices;
        std::vector<Vertex> vertices;
        u32 numUVChannels;
        u32 numColorChannels;
        bool hasTangents;
        bool hasBoneWeights;
    };

    vec3 aabbMin;
    vec3 aabbMax;
    std::vector<Mesh> meshes;

    bool Load( std::string_view filename );
    bool Save( std::string_view filename, u32 floatPrecision = 6, bool logProgress = true );

    void CalculateAABB();
    void PrintInfo( std::string tabLevel = "" ) const;
    PModelVersionNum GetLoadedVersionNum() const { return m_loadedVersionNum; }

private:
    PModelVersionNum m_loadedVersionNum;

    bool LoadBinary( std::string_view filename );
    bool LoadText( std::string_view filename );

    bool SaveBinary( std::string_view filename ) const;
    bool SaveText( std::string_view filename, u32 floatPrecision = 6, bool logProgress = true ) const;
};

std::vector<std::string> GetUsedMaterialsPModel( const std::string& filename );

} // namespace PG
