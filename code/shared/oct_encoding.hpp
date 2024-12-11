#pragma once

#include "shared/math_vec.hpp"

inline f32 SignNotZero( const f32& x ) { return ( x >= 0.0f ) ? 1.0f : -1.0f; }

inline vec2 SignNotZero( const vec2& v ) { return { SignNotZero( v.x ), SignNotZero( v.y ) }; }

// https://jcgt.org/published/0003/02/01/ and
// https://www.shadertoy.com/view/Mtfyzl
vec2 Vec3ToOct( const vec3& v, u32 numBits )
{
    vec2 oct = vec2( v.x, v.y ) * ( 1.0f / ( Abs( v.x ) + Abs( v.y ) + Abs( v.z ) ) );
    oct      = ( v.z <= 0.0f ) ? ( ( 1.0f - Abs( vec2( oct.y, oct.x ) ) ) * SignNotZero( oct ) ) : oct;
    return oct;
}

vec3 OctToVec3( const vec2& oct )
{
#if 0
    vec3 v = vec3( oct, 1.0f - Abs( oct.x ) - Abs( oct.y ) );
    if ( v.z < 0.0f )
    {
        vec2 copy = v;
        v.x       = ( 1.0f - Abs( copy.y ) ) * SignNotZero( v.x );
        v.y       = ( 1.0f - Abs( copy.x ) ) * SignNotZero( v.y );
    }
#else
    vec3 v  = vec3( oct, 1.0f - Abs( oct.x ) - Abs( oct.y ) );
    float t = Max( -v.z, 0.0f );
    v.x += ( v.x > 0.0f ) ? -t : t;
    v.y += ( v.y > 0.0f ) ? -t : t;
#endif
    return Normalize( v );
}

u32 OctEncodeUNorm( const vec3& v, u32 numBits )
{
    vec2 oct = Saturate( 0.5f * Vec3ToOct( v, numBits ) + 0.5f );
    u32 x    = QuantizeUNorm( oct.x, numBits );
    u32 y    = QuantizeUNorm( oct.y, numBits );
    return ( y << numBits ) | x;
}

u32 OctEncodeUNorm_P( const vec3& v, u32 numBits )
{
    vec2 oct       = Saturate( 0.5f * Vec3ToOct( v, numBits ) + 0.5f );
    f32 m          = f32( ( 1u << numBits ) - 1u );
    vec2 s         = Floor( m * oct ) / m;
    vec2 bestOct   = s;
    f32 bestCosine = dot( v, OctToVec3( 2.0f * s - 1.0f ) );

    for ( int i = 0; i <= 1; ++i )
    {
        for ( int j = 0; j <= 1; ++j )
        {
            if ( i != 0 || j != 0 )
            {
                vec2 newOct     = s + vec2( i, j ) / m;
                float newCosine = dot( v, OctToVec3( 2.0f * newOct - 1.0f ) );
                if ( newCosine > bestCosine )
                {
                    bestCosine = newCosine;
                    bestOct    = newOct;
                }
            }
        }
    }

    u32 x = QuantizeUNorm( bestOct.x, numBits );
    u32 y = QuantizeUNorm( bestOct.y, numBits );
    return ( y << numBits ) | x;
}

u32 OctEncodeSNorm( const vec3& v, u32 numBits )
{
    vec2 oct = Clamp( Vec3ToOct( v, numBits ), vec2( -1.0f ), vec2( 1.0f ) );
    u32 x    = QuantizeSNorm( oct.x, numBits );
    u32 y    = QuantizeSNorm( oct.y, numBits );
    return ( y << numBits ) | x;
}

u32 OctEncodeSNorm_P( const vec3& v, u32 numBits )
{
    vec2 oct       = Clamp( Vec3ToOct( v, numBits ), vec2( -1.0f ), vec2( 1.0f ) );
    f32 m          = f32( ( 1u << ( numBits - 1 ) ) - 1u );
    vec2 s         = Floor( m * oct ) / m;
    vec2 bestOct   = s;
    f32 bestCosine = dot( v, OctToVec3( s ) );

    for ( int i = 0; i <= 1; ++i )
    {
        for ( int j = 0; j <= 1; ++j )
        {
            if ( i != 0 || j != 0 )
            {
                vec2 newOct     = s + vec2( i, j ) / m;
                float newCosine = dot( v, OctToVec3( newOct ) );
                if ( newCosine > bestCosine )
                {
                    bestCosine = newCosine;
                    bestOct    = newOct;
                }
            }
        }
    }

    u32 x = QuantizeSNorm( bestOct.x, numBits );
    u32 y = QuantizeSNorm( bestOct.y, numBits );
    return ( y << numBits ) | x;
}

vec3 OctDecodeUNorm( u32 packed, u32 numBits )
{
    u32 m  = ( 1u << numBits ) - 1u;
    u32 x  = packed & m;
    u32 y  = ( packed >> numBits ) & m;
    vec2 e = vec2( x, y ) / float( m );

    return OctToVec3( 2.0f * e - 1.0f );
}

vec3 OctDecodeSNorm( u32 packed, u32 numBits )
{
    u32 m  = ( 1u << numBits ) - 1u;
    i32 mh = ( 1 << ( numBits - 1 ) ) - 1;
    i32 x  = packed & m;
    i32 y  = ( packed >> numBits ) & m;
    vec2 e = vec2( x - mh, y - mh ) / float( mh );

    return OctToVec3( e );
}
