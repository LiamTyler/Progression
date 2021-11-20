#include "anti_aliasing.hpp"
#include "shared/core_defines.hpp"
#include "shared/logger.hpp"
#include "shared/random.hpp"
#include <unordered_map>

using namespace PG::Random;

namespace PT
{
namespace AntiAlias
{

Algorithm AlgorithmFromString( const std::string& alg )
{
    std::unordered_map< std::string, Algorithm > map =
    {
        { "NONE",             Algorithm::NONE },
        { "REGULAR_2X2_GRID", Algorithm::REGULAR_2X2_GRID },
        { "REGULAR_4X4_GRID", Algorithm::REGULAR_4X4_GRID },
        { "ROTATED_2X2_GRID", Algorithm::ROTATED_2X2_GRID },
        { "JITTER_5",         Algorithm::JITTER_5 },
    };

    auto it = map.find( alg );
    if ( it == map.end() )
    {
        LOG( "Antialiasing algorithm '%s' is not a valid option!", alg.c_str() );
        return Algorithm::NONE;
    }

    return it->second;
}

glm::vec3 None( int iteration, const glm::vec3& pixelCenter, const glm::vec3& dU, const glm::vec3& dV )
{
    return pixelCenter;
}

glm::vec3 Regular2x2Grid( int iteration, const glm::vec3& pixelCenter, const glm::vec3& dU, const glm::vec3& dV )
{
    static glm::vec2 offsets[] =
    {
        { -0.25, -0.25 },
        { 0.25,  -0.25 },
        { 0.25,   0.25 },
        { -0.25,  0.25 },
    };

    return pixelCenter + offsets[iteration].x * dU + offsets[iteration].y * dV;
}

glm::vec3 Regular4x4Grid( int iteration, const glm::vec3& pixelCenter, const glm::vec3& dU, const glm::vec3& dV )
{
    static glm::vec2 offsets[] =
    {
        { -0.375, -0.375 },
        { -0.125, -0.375 },
        { 0.125,  -0.375 },
        { 0.375,  -0.375 },
        { -0.375, -0.125 },
        { -0.125, -0.125 },
        { 0.125,  -0.125 },
        { 0.375,  -0.125 },
        { -0.375,  0.125 },
        { -0.125,  0.125 },
        { 0.125,   0.125 },
        { 0.375,   0.125 },
        { -0.375,  0.375 },
        { -0.125,  0.375 },
        { 0.125,   0.375 },
        { 0.375,   0.375 },
    };

    return pixelCenter + offsets[iteration].x * dU + offsets[iteration].y * dV;
}

glm::vec3 Rotated2x2Grid( int iteration, const glm::vec3& pixelCenter, const glm::vec3& dU, const glm::vec3& dV )
{
    static glm::vec2 offsets[] =
    {
        { -0.375, -0.125 },
        { 0.125,  -0.375 },
        { 0.375,   0.125 },
        { -0.125,  0.375 },
    };

    return pixelCenter + offsets[iteration].x * dU + offsets[iteration].y * dV;
}

glm::vec3 Jitter( int iteration, const glm::vec3& pixelCenter, const glm::vec3& dU, const glm::vec3& dV )
{
    return pixelCenter + RandFloat( -0.5f, 0.5f ) * dU + RandFloat( -0.5f, 0.5f ) * dV;
}

int GetIterations( Algorithm alg )
{
    static int iterations[] =
    {
        1,  // NONE
        4,  // REGULAR_2X2_GRID
        16, // REGULAR_4X4_GRID
        4,  // ROTATED_2X2_GRID
        5,  // JITTER_5
    };
    static_assert( ARRAY_COUNT( iterations ) == static_cast< int >( Algorithm::NUM_ALGORITHM ), "Forgot to update this" );

    return iterations[static_cast< int >( alg )];
}

AAFuncPointer GetAlgorithm( Algorithm alg )
{
    static AAFuncPointer functions[] =
    {
        None,            // NONE
        Regular2x2Grid,  // REGULAR_2X2_GRID
        Regular4x4Grid,  // REGULAR_4X4_GRID
        Rotated2x2Grid,  // ROTATED_2X2_GRID
        Jitter,          // JITTER_5
    };
    static_assert( ARRAY_COUNT( functions ) == static_cast< int >( Algorithm::NUM_ALGORITHM ), "Forgot to update this" );

    return functions[static_cast< int >( alg )];
}

} // namespace AntiAlias
} // namespace PT