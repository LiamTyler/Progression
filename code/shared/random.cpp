#include "shared/random.hpp"
#include <stdlib.h>
#include <time.h>

namespace PG
{
namespace Random
{

    void SetSeed( size_t seed )
    {
        srand( (unsigned int) seed );
    }

    int RandInt( int l, int h )
    {
        return l + rand() % ( h - l + 1 );
    }

    float RandFloat( float l, float h )
    {
        float r = rand() / static_cast< float >( RAND_MAX );
        return r * ( h - l ) + l;
    }

    float Rand() { return rand() / static_cast< float >( RAND_MAX ); }

} // namespace Random

} // namespace PG
