#include "anti_aliasing.hpp"
#include "shared/core_defines.hpp"
#include "shared/logger.hpp"
#include "shared/random.hpp"
#include <unordered_map>

using namespace PG::Random;

namespace PT::AntiAlias
{

Algorithm AlgorithmFromString( const std::string& alg )
{
    std::unordered_map<std::string, Algorithm> map = {
        {"NONE",             Algorithm::NONE            },
        {"REGULAR_2X2_GRID", Algorithm::REGULAR_2X2_GRID},
        {"REGULAR_4X4_GRID", Algorithm::REGULAR_4X4_GRID},
        {"ROTATED_2X2_GRID", Algorithm::ROTATED_2X2_GRID},
        {"JITTER",           Algorithm::JITTER          },
    };

    auto it = map.find( alg );
    if ( it == map.end() )
    {
        LOG_WARN( "Antialiasing algorithm '%s' is not a valid option!", alg.c_str() );
        return Algorithm::NONE;
    }

    return it->second;
}

vec2 None( i32 iteration, RNG& rng ) { return { 0, 0 }; }

vec2 Regular2x2Grid( i32 iteration, RNG& rng )
{
    static vec2 offsets[] = {
        {-0.25, -0.25},
        {0.25,  -0.25},
        {0.25,  0.25 },
        {-0.25, 0.25 },
    };

    return offsets[iteration % 4];
}

vec2 Regular4x4Grid( i32 iteration, RNG& rng )
{
    static vec2 offsets[] = {
        {-0.375, -0.375},
        {-0.125, -0.375},
        {0.125,  -0.375},
        {0.375,  -0.375},
        {-0.375, -0.125},
        {-0.125, -0.125},
        {0.125,  -0.125},
        {0.375,  -0.125},
        {-0.375, 0.125 },
        {-0.125, 0.125 },
        {0.125,  0.125 },
        {0.375,  0.125 },
        {-0.375, 0.375 },
        {-0.125, 0.375 },
        {0.125,  0.375 },
        {0.375,  0.375 },
    };

    return offsets[iteration % 16];
}

vec2 Rotated2x2Grid( i32 iteration, RNG& rng )
{
    static vec2 offsets[] = {
        {-0.375, -0.125},
        {0.125,  -0.375},
        {0.375,  0.125 },
        {-0.125, 0.375 },
    };

    return offsets[iteration % 4];
}

vec2 Jitter( i32 iteration, RNG& rng ) { return { rng.UniformFloat() - 0.5f, rng.UniformFloat() - 0.5f }; }

i32 GetIterations( Algorithm alg )
{
    static i32 iterations[] = {
        1,  // NONE
        4,  // REGULAR_2X2_GRID
        16, // REGULAR_4X4_GRID
        4,  // ROTATED_2X2_GRID
        0,  // JITTER
    };
    static_assert( ARRAY_COUNT( iterations ) == static_cast<i32>( Algorithm::NUM_ALGORITHM ), "Forgot to update this" );

    return iterations[static_cast<i32>( alg )];
}

AAFuncPointer GetAlgorithm( Algorithm alg )
{
    static AAFuncPointer functions[] = {
        None,           // NONE
        Regular2x2Grid, // REGULAR_2X2_GRID
        Regular4x4Grid, // REGULAR_4X4_GRID
        Rotated2x2Grid, // ROTATED_2X2_GRID
        Jitter,         // JITTER
    };
    static_assert( ARRAY_COUNT( functions ) == static_cast<i32>( Algorithm::NUM_ALGORITHM ), "Forgot to update this" );

    return functions[static_cast<i32>( alg )];
}

} // namespace PT::AntiAlias
