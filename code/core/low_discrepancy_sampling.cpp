#include "low_discrepancy_sampling.hpp"

namespace PG
{

static uint32_t ReverseBits32( uint32_t bits ) 
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return bits;
}

static uint64_t ReverseBits64( uint64_t bits ) 
{
    uint64_t n0 = ReverseBits32( (uint32_t)bits );
    uint64_t n1 = ReverseBits32( (uint32_t)(bits >> 32) );
    return (n0 << 32) | n1;
}


float Hammersley32( uint32_t index )
{
    uint32_t reversed = ReverseBits32( index );
    return reversed * 0x1p-32f;
}


double Hammersley64( uint64_t index )
{
    uint64_t reversed = ReverseBits64( index );
    return reversed * 0x1p-64;
}

} // namespace PG