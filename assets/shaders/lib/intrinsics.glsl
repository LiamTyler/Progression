#ifndef __INTRINSICS_GLSL__
#define __INTRINSICS_GLSL__

// Largely copied from FidelityFX SDK

#extension GL_KHR_shader_subgroup_arithmetic : require
#extension GL_KHR_shader_subgroup_ballot : require
#extension GL_KHR_shader_subgroup_quad : require
#extension GL_KHR_shader_subgroup_shuffle : require

uint WaveGetLaneCount()
{
    return gl_SubgroupSize;
}

bool WaveIsFirstLane()
{
    return subgroupElect();
}

uint WaveLaneIndex()
{
    return gl_SubgroupInvocationID;
}

bool WaveReadAtLaneIndexB1( bool v, uint x )
{
    return subgroupShuffle(v, x);
}

uint WavePrefixCountBits( bool v )
{
    return subgroupBallotExclusiveBitCount( subgroupBallot( v ) );
}

uint WaveActiveCountBits( bool v )
{
    return subgroupBallotBitCount( subgroupBallot( v ) );
}

#define WaveReadLaneFirst( val ) subgroupBroadcastFirst( val )
#define WaveReadLaneAt( val, lane ) subgroupBroadcast( val, lane )

#define WaveActiveSum( val ) subgroupAdd( val )
#define WaveActiveProduct( val ) subgroupMul( val )
#define WaveActiveMin( val ) subgroupMin( val )
#define WaveActiveMax( val ) subgroupMax( val )
#define WaveActiveBitAnd( val ) subgroupAnd( val )
#define WaveActiveBitOr( val ) subgroupOr( val )
#define WaveActiveBitXor( val ) subgroupXor( val )

#define WavePrefixSum( val ) subgroupExclusiveAdd( val )
#define WavePrefixProduct( val ) subgroupExclusiveMul( val )

#endif // #ifndef __INTRINSICS_GLSL__