#include "asset/types/font.hpp"
#include "image.hpp"
#include "shared/bitops.hpp"
#include "shared/filesystem.hpp"
#include "shared/logger.hpp"
#include "shared/lz4_compressor.hpp"
#include "shared/oct_encoding.hpp"
#include "shared/random.hpp"
#include <ctime>
#include <iostream>
#include <random>

using namespace PG;
// using namespace std;

int main( int argc, char* argv[] )
{
    Logger_Init();
    Logger_AddLogLocation( "stdout", stdout );

    Random::RNG rng;

    const u32 BITS       = 8; // per axis, so 2 * BITS in total
    const u32 ITERATIONS = 5'000'000;

    double totalError = 0;
    double maxError   = 0;
    std::mt19937 mt( (u32)time( nullptr ) );
    std::uniform_real_distribution<f32> unif( -1, 1 );

    for ( u32 i = 0; i < ITERATIONS; ++i )
    {
        vec3 n = 2.0f * vec3( rng.UniformFloat(), rng.UniformFloat(), rng.UniformFloat() ) - 1.0f;
        // vec3 n = vec3( unif( mt ), unif( mt ), unif( mt ) );
        n = Normalize( n );

        u32 quantized = OctEncodeUNorm_P( n, BITS );
        vec3 qn       = OctDecodeUNorm( quantized, BITS );

        // u32 quantized = OctEncodeSNorm_P( n, BITS );
        // vec3 qn       = OctDecodeSNorm( quantized, BITS );

        dvec3 dn    = glm::normalize( dvec3( n ) );
        dvec3 dqn   = glm::normalize( dvec3( qn ) );
        double diff = abs( std::acos( Saturate( glm::dot( dn, dqn ) ) ) );
        maxError    = Max( maxError, (double)diff );
        totalError += diff;
    }
    maxError *= 180.0 / PI_D;
    totalError /= ITERATIONS;
    totalError *= 180.0 / PI_D;

    LOG( "Bits: %u, avg error: %f, max error: %f", BITS, totalError, maxError );

    return 0;
}
