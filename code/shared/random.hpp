#pragma once

#include <cstdint>
#include <algorithm>

namespace PG::Random
{

// A PCG implementation, from PBRT-V3: https://pbr-book.org/3ed-2018/Utilities/Main_Include_File#fragment-RNGPublicMethods-3
static const double DoubleOneMinusEpsilon = 0x1.fffffffffffffp-1;
static const float FloatOneMinusEpsilon = 0x1.fffffep-1;

#define PCG32_DEFAULT_STATE 0x853c49e6748fea9bULL
#define PCG32_DEFAULT_STREAM 0xda3e39cb94b95bdbULL
#define PCG32_MULT 0x5851f42d4c957f2dULL
class RNG
{
public:
    RNG();
    RNG( uint64_t sequenceIndex ) { SetSequence( sequenceIndex ); }
    void SetSequence( uint64_t sequenceIndex );
    uint32_t UniformUInt32();
    uint32_t UniformUInt32( uint32_t upperBound )
    {
        uint32_t threshold = (~upperBound + 1u) % upperBound;
        while ( true )
        {
            uint32_t r = UniformUInt32();
            if ( r >= threshold ) return r % upperBound;
        }
    }

    float UniformFloat()
    {
        return std::min( FloatOneMinusEpsilon, float(UniformUInt32() * 0x1p-32f) );
    }

private:
    uint64_t state, inc;
};

inline RNG::RNG() : state(PCG32_DEFAULT_STATE), inc(PCG32_DEFAULT_STREAM) {}
inline void RNG::SetSequence( uint64_t initseq )
{
    state = 0u;
    inc = (initseq << 1u) | 1u;
    UniformUInt32();
    state += PCG32_DEFAULT_STATE;
    UniformUInt32();
}

inline uint32_t RNG::UniformUInt32()
{
    uint64_t oldstate = state;
    state = oldstate * PCG32_MULT + inc;
    uint32_t xorshifted = (uint32_t)(((oldstate >> 18u) ^ oldstate) >> 27u);
    uint32_t rot = (uint32_t)(oldstate >> 59u);
    return (xorshifted >> rot) | (xorshifted << ((~rot + 1u) & 31));
}

} // namespace PG::Random