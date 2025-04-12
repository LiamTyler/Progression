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

Random::RNG rng;

vec3 GetRandomVec3()
{
    vec3 n = 2.0f * vec3( rng.UniformFloat(), rng.UniformFloat(), rng.UniformFloat() ) - 1.0f;
    return Normalize( n );
}

double GetAngleDiff( const vec3& a, const vec3& b )
{
    dvec3 da = glm::normalize( dvec3( a ) );
    dvec3 db = glm::normalize( dvec3( b ) );
    return abs( std::acos( Saturate( glm::dot( da, db ) ) ) );
}

void TestMethod()
{
    const u32 BITS       = 8; // per axis, so 2 * BITS in total
    const u32 ITERATIONS = 1'000'000;

    double totalError = 0;
    double maxError   = 0;

    for ( u32 i = 0; i < ITERATIONS; ++i )
    {
        vec3 n = GetRandomVec3();

        u32 quantized = OctEncodeUNorm( n, BITS );
        vec3 qn       = OctDecodeUNorm( quantized, BITS );

        double diff = GetAngleDiff( n, qn );
        maxError    = Max( maxError, (double)diff );
        totalError += diff;
    }
    maxError *= 180.0 / PI_D;
    totalError /= ITERATIONS;
    totalError *= 180.0 / PI_D;

    LOG( "Bits: %u, avg error: %f, max error: %f", BITS, totalError, maxError );
}

vec2 Vec3ToOct_Z_Up( const vec3& v )
{
    vec2 oct = vec2( v.x, v.y ) * ( 1.0f / ( Abs( v.x ) + Abs( v.y ) + Abs( v.z ) ) );
    oct      = ( v.z <= 0.0f ) ? ( ( 1.0f - Abs( vec2( oct.y, oct.x ) ) ) * SignNotZero( oct ) ) : oct;
    return oct;
}

vec2 Vec3ToOct_Z_Down( const vec3& v )
{
    vec2 oct = vec2( v.x, v.y ) * ( 1.0f / ( Abs( v.x ) + Abs( v.y ) + Abs( v.z ) ) );
    oct      = ( v.z > 0.0f ) ? ( ( 1.0f - Abs( vec2( oct.y, oct.x ) ) ) * SignNotZero( oct ) ) : oct;
    return oct;
}

vec2 Vec3ToOct_X_Up( const vec3& v )
{
    vec2 oct = vec2( v.y, v.z ) * ( 1.0f / ( Abs( v.x ) + Abs( v.y ) + Abs( v.z ) ) );
    oct      = ( v.x <= 0.0f ) ? ( ( 1.0f - Abs( vec2( oct.y, oct.x ) ) ) * SignNotZero( oct ) ) : oct;
    return oct;
}

vec2 Vec3ToOct_X_Down( const vec3& v )
{
    vec2 oct = vec2( v.y, v.z ) * ( 1.0f / ( Abs( v.x ) + Abs( v.y ) + Abs( v.z ) ) );
    oct      = ( v.x > 0.0f ) ? ( ( 1.0f - Abs( vec2( oct.y, oct.x ) ) ) * SignNotZero( oct ) ) : oct;
    return oct;
}

vec2 Vec3ToOct_Y_Up( const vec3& v )
{
    vec2 oct = vec2( v.x, v.z ) * ( 1.0f / ( Abs( v.x ) + Abs( v.y ) + Abs( v.z ) ) );
    oct      = ( v.y <= 0.0f ) ? ( ( 1.0f - Abs( vec2( oct.y, oct.x ) ) ) * SignNotZero( oct ) ) : oct;
    return oct;
}

vec2 Vec3ToOct_Y_Down( const vec3& v )
{
    vec2 oct = vec2( v.x, v.z ) * ( 1.0f / ( Abs( v.x ) + Abs( v.y ) + Abs( v.z ) ) );
    oct      = ( v.y > 0.0f ) ? ( ( 1.0f - Abs( vec2( oct.y, oct.x ) ) ) * SignNotZero( oct ) ) : oct;
    return oct;
}

vec3 OctToVec3_Z_Up( const vec2& oct )
{
    vec3 v  = vec3( oct, 1.0f - Abs( oct.x ) - Abs( oct.y ) );
    float t = Max( -v.z, 0.0f );
    v.x += ( v.x > 0.0f ) ? -t : t;
    v.y += ( v.y > 0.0f ) ? -t : t;
    return Normalize( v );
}

vec3 OctToVec3_Z_Down( const vec2& oct )
{
    vec3 v  = vec3( oct, -1.0f + Abs( oct.x ) + Abs( oct.y ) );
    float t = Min( -v.z, 0.0f );
    v.x += ( v.x > 0.0f ) ? t : -t;
    v.y += ( v.y > 0.0f ) ? t : -t;
    return Normalize( v );
}

vec3 OctToVec3_X_Up( const vec2& oct )
{
    vec3 v  = vec3( 1.0f - Abs( oct.x ) - Abs( oct.y ), oct );
    float t = Max( -v.x, 0.0f );
    v.y += ( v.y > 0.0f ) ? -t : t;
    v.z += ( v.z > 0.0f ) ? -t : t;
    return Normalize( v );
}

vec3 OctToVec3_X_Down( const vec2& oct )
{
    vec3 v  = vec3( -1.0f + Abs( oct.x ) + Abs( oct.y ), oct );
    float t = Min( -v.x, 0.0f );
    v.y += ( v.y > 0.0f ) ? t : -t;
    v.z += ( v.z > 0.0f ) ? t : -t;
    return Normalize( v );
}

vec3 OctToVec3_Y_Up( const vec2& oct )
{
    vec3 v  = vec3( oct.x, 1.0f - Abs( oct.x ) - Abs( oct.y ), oct.y );
    float t = Max( -v.y, 0.0f );
    v.x += ( v.x > 0.0f ) ? -t : t;
    v.z += ( v.z > 0.0f ) ? -t : t;
    return Normalize( v );
}

vec3 OctToVec3_Y_Down( const vec2& oct )
{
    vec3 v  = vec3( oct.x, -1.0f + Abs( oct.x ) + Abs( oct.y ), oct.y );
    float t = Min( -v.y, 0.0f );
    v.x += ( v.x > 0.0f ) ? t : -t;
    v.z += ( v.z > 0.0f ) ? t : -t;
    return Normalize( v );
}

bool WithinEps( f32 x, f32 target, f32 eps ) { return abs( x - target ) < eps; }

vec4 GetColorFromDir( const vec3& d )
{
    static const float EPS = 0.004f;
    if ( WithinEps( d.x, 0, EPS ) || WithinEps( d.y, 0, EPS ) || WithinEps( d.z, 0, EPS ) )
        return vec4( 0, 0, 0, 1 );

    static const vec4 colors[8] = {
        vec4( 238, 235, 78, 255 ) / 255.0f,
        vec4( 237, 100, 63, 255 ) / 255.0f,
        vec4( 249, 248, 245, 255 ) / 255.0f,
        vec4( 193, 136, 40, 255 ) / 255.0f,
        vec4( 125, 221, 136, 255 ) / 255.0f,
        vec4( 163, 197, 221, 255 ) / 255.0f,
        vec4( 231, 241, 245, 255 ) / 255.0f,
        vec4( 197, 182, 211, 255 ) / 255.0f,
    };

    int idx = 4 * ( d.z < 0 ) + 2 * ( d.y >= 0 ) + 1 * ( d.x >= 0 );
    return colors[idx];
}

enum OctaOrientation
{
    OCTA_Z_UP,
    OCTA_Z_DOWN,
    OCTA_X_UP,
    OCTA_X_DOWN,
    OCTA_Y_UP,
    OCTA_Y_DOWN,

    NUM_ORIENTATIONS
};

const std::string orientationNames[] = {
    "Z_Up",
    "Z_Down",
    "X_Up",
    "X_Down",
    "Y_Up",
    "Y_Down",
};

using EncodeFunc = vec2 ( * )( const vec3& );
using DecodeFunc = vec3 ( * )( const vec2& );

const EncodeFunc encodeFuncs[] = {
    Vec3ToOct_Z_Up,
    Vec3ToOct_Z_Down,
    Vec3ToOct_X_Up,
    Vec3ToOct_X_Down,
    Vec3ToOct_Y_Up,
    Vec3ToOct_Y_Down,
};

const DecodeFunc decodeFuncs[] = {
    OctToVec3_Z_Up,
    OctToVec3_Z_Down,
    OctToVec3_X_Up,
    OctToVec3_X_Down,
    OctToVec3_Y_Up,
    OctToVec3_Y_Down,
};

RawImage2D GenerateOctaDiagram( OctaOrientation orientation, int imgSize )
{
    auto decodeFn = decodeFuncs[orientation];
    FloatImage2D octaImg( imgSize, imgSize, 3 );
#pragma omp parallel for
    for ( int row = 0; row < imgSize; ++row )
    {
        for ( int col = 0; col < imgSize; ++col )
        {
            vec2 texelPos = vec2( col + 0.5f, row + 0.5f ) / (float)imgSize;
            vec2 octaPos  = texelPos * 2.0f - 1.0f;
            octaPos.y *= -1;

            vec3 dir = decodeFn( octaPos );
            octaImg.SetFromFloat4( row, col, GetColorFromDir( dir ) );
        }
    }

    RawImage2D img = RawImage2DFromFloatImage( octaImg, ImageFormat::R8_G8_B8_UNORM );
    return img;
}

void GenerateAllOctaDiagrams( int imgSize )
{
    std::string directory = PG_ROOT_DIR "octa/";
    for ( int i = 0; i < NUM_ORIENTATIONS; ++i )
    {
        RawImage2D img = GenerateOctaDiagram( (OctaOrientation)i, imgSize );
        img.Save( directory + orientationNames[i] + ".png" );
    }
}

int main( int argc, char* argv[] )
{
    Logger_Init();
    Logger_AddLogLocation( "stdout", stdout );

    GenerateAllOctaDiagrams( 512 );

    double totalError = 0;
    double maxError   = 0;
    u32 ITERATIONS    = 1000000;
    for ( u32 i = 0; i < ITERATIONS; ++i )
    {
        vec3 n = GetRandomVec3();

        vec2 octa = Vec3ToOct_Y_Down( n );
        vec3 rn   = OctToVec3_Y_Down( octa );

        double diff = GetAngleDiff( n, rn );
        maxError    = Max( maxError, (double)diff );
        totalError += diff;
    }
    maxError *= 180.0 / PI_D;
    totalError /= ITERATIONS;
    totalError *= 180.0 / PI_D;

    LOG( "Avg error: %f, max error: %f", totalError, maxError );

    return 0;
}
