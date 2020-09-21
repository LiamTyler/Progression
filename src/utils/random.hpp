#pragma once

#include <cstddef>

namespace PG
{
namespace Random
{

    /**
     * \brief Sets the seed for the random number generator
     */
    void SetSeed( size_t seed );

    /**
     * \brief Returns a random int in the range [low, high]
     */
    int RandInt( int low, int high );

    /**
     * \brief Returns a random float in the range [low, high]
     */
    float RandFloat( float low, float high );

    /**
     * \brief Returns a random float in the range [0, 1]
     */
    float Rand();

} // namespace Random
} // namespace PG
