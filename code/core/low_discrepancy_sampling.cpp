#include "low_discrepancy_sampling.hpp"

namespace PG
{

static u32 ReverseBits32( u32 bits )
{
    bits = ( bits << 16u ) | ( bits >> 16u );
    bits = ( ( bits & 0x55555555u ) << 1u ) | ( ( bits & 0xAAAAAAAAu ) >> 1u );
    bits = ( ( bits & 0x33333333u ) << 2u ) | ( ( bits & 0xCCCCCCCCu ) >> 2u );
    bits = ( ( bits & 0x0F0F0F0Fu ) << 4u ) | ( ( bits & 0xF0F0F0F0u ) >> 4u );
    bits = ( ( bits & 0x00FF00FFu ) << 8u ) | ( ( bits & 0xFF00FF00u ) >> 8u );
    return bits;
}

static u64 ReverseBits64( u64 bits )
{
    u64 n0 = ReverseBits32( (u32)bits );
    u64 n1 = ReverseBits32( (u32)( bits >> 32 ) );
    return ( n0 << 32 ) | n1;
}

f32 Hammersley32( u32 index )
{
    u32 reversed = ReverseBits32( index );
    return reversed * 0x1p-32f;
}

f64 Hammersley64( u64 index )
{
    u64 reversed = ReverseBits64( index );
    return reversed * 0x1p-64;
}

} // namespace PG
