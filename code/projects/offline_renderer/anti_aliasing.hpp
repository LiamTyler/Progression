#pragma once

#include "shared/math.hpp"
#include "shared/random.hpp"
#include <functional>

namespace PT::AntiAlias
{

typedef glm::vec2 ( *AAFuncPointer )( int iteration, PG::Random::RNG& rng );

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

// return an offsets from the pixel center (-0.5 to 0.5)
glm::vec2 None( int iteration, PG::Random::RNG& rng );
glm::vec2 Regular2x2Grid( int iteration, PG::Random::RNG& rng );
glm::vec2 Regular4x4Grid( int iteration, PG::Random::RNG& rng );
glm::vec2 Rotated2x2Grid( int iteration, PG::Random::RNG& rng );
glm::vec2 Jitter( int iteration, PG::Random::RNG& rng );

int GetIterations( Algorithm alg );

AAFuncPointer GetAlgorithm( Algorithm );

} // namespace PT::AntiAlias
