#pragma once

#include "shared/core_defines.hpp"
#include <algorithm>

namespace PG::Random
{

// A PCG implementation, from PBRT-V3: https://pbr-book.org/3ed-2018/Utilities/Main_Include_File#fragment-RNGPublicMethods-3
static const f64 DoubleOneMinusEpsilon = 0x1.fffffffffffffp-1;
static const f32 FloatOneMinusEpsilon  = 0x1.fffffep-1;

#define PCG32_DEFAULT_STATE 0x853c49e6748fea9bULL
#define PCG32_DEFAULT_STREAM 0xda3e39cb94b95bdbULL
#define PCG32_MULT 0x5851f42d4c957f2dULL
class RNG
{
public:
    RNG();
    RNG( u64 sequenceIndex ) { SetSequence( sequenceIndex ); }
    void SetSequence( u64 sequenceIndex );
    u32 UniformUInt32();
    u32 UniformUInt32( u32 upperBound )
    {
        u32 threshold = ( ~upperBound + 1u ) % upperBound;
        while ( true )
        {
            u32 r = UniformUInt32();
            if ( r >= threshold )
                return r % upperBound;
        }
    }

    f32 UniformFloat() { return std::min( FloatOneMinusEpsilon, f32( UniformUInt32() * 0x1p-32f ) ); }

private:
    u64 state, inc;
};

inline RNG::RNG() : state( PCG32_DEFAULT_STATE ), inc( PCG32_DEFAULT_STREAM ) {}
inline void RNG::SetSequence( u64 initseq )
{
    state = 0u;
    inc   = ( initseq << 1u ) | 1u;
    UniformUInt32();
    state += PCG32_DEFAULT_STATE;
    UniformUInt32();
}

inline u32 RNG::UniformUInt32()
{
    u64 oldstate   = state;
    state          = oldstate * PCG32_MULT + inc;
    u32 xorshifted = (u32)( ( ( oldstate >> 18u ) ^ oldstate ) >> 27u );
    u32 rot        = (u32)( oldstate >> 59u );
    return ( xorshifted >> rot ) | ( xorshifted << ( ( ~rot + 1u ) & 31 ) );
}

} // namespace PG::Random
