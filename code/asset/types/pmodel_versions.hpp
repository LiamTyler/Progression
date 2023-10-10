#pragma once

namespace PG
{

enum class PModelVersionNum : unsigned int
{
    BITANGENT_SIGNS = 1,
    VERTEX_DATA_TOGETHER = 2,

    TOTAL,
    CURRENT_VERSION = TOTAL - 1,
    LAST_SUPPORTED_VERSION = VERTEX_DATA_TOGETHER,
};

#define PMODEL_MAX_UVS_PER_VERT 8u           // AI_MAX_NUMBER_OF_TEXTURECOORDS
#define PMODEL_MAX_COLORS_PER_VERT 8u        // AI_MAX_NUMBER_OF_COLOR_SETS
#define PMODEL_MAX_BONE_WEIGHTS_PER_VERT 16u // Assimp's is INT_MAX

} // namespace PG