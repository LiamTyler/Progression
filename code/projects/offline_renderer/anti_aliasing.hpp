#pragma once

#include "shared/math.hpp"
#include <functional>

namespace PT
{
namespace AntiAlias
{

typedef glm::vec3 (*AAFuncPointer)( int iteration, const glm::vec3& pixelCenter, const glm::vec3& dU, const glm::vec3& dV );

enum class Algorithm
{
    NONE,
    REGULAR_2X2_GRID,
    REGULAR_4X4_GRID,
    ROTATED_2X2_GRID,
    JITTER,

    NUM_ALGORITHM
};

Algorithm AlgorithmFromString( const std::string& alg );

glm::vec3 None( int iteration, const glm::vec3& pixelCenter, const glm::vec3& dU, const glm::vec3& dV );

glm::vec3 Regular2x2Grid( int iteration, const glm::vec3& pixelCenter, const glm::vec3& dU, const glm::vec3& dV );

glm::vec3 Regular4x4Grid( int iteration, const glm::vec3& pixelCenter, const glm::vec3& dU, const glm::vec3& dV );

glm::vec3 Rotated2x2Grid( int iteration, const glm::vec3& pixelCenter, const glm::vec3& dU, const glm::vec3& dV );

glm::vec3 Jitter( int iteration, const glm::vec3& pixelCenter, const glm::vec3& dU, const glm::vec3& dV );

int GetIterations( Algorithm alg );

AAFuncPointer GetAlgorithm( Algorithm );

} // namespace AntiAlias
} // namespace PT