#pragma once

#include "shared/math_vec.hpp"
#include "shared/random.hpp"
#include <functional>
#include <string>

namespace PT::AntiAlias
{

typedef vec2 ( *AAFuncPointer )( i32 iteration, PG::Random::RNG& rng );

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
vec2 None( i32 iteration, PG::Random::RNG& rng );
vec2 Regular2x2Grid( i32 iteration, PG::Random::RNG& rng );
vec2 Regular4x4Grid( i32 iteration, PG::Random::RNG& rng );
vec2 Rotated2x2Grid( i32 iteration, PG::Random::RNG& rng );
vec2 Jitter( i32 iteration, PG::Random::RNG& rng );

i32 GetIterations( Algorithm alg );

AAFuncPointer GetAlgorithm( Algorithm );

} // namespace PT::AntiAlias
