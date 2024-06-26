#ifndef __INTRINSICS_GLSL__
#define __INTRINSICS_GLSL__

// Largely copied from FidelityFX SDK

#extension GL_KHR_shader_subgroup_arithmetic : require
#extension GL_KHR_shader_subgroup_ballot : require
#extension GL_KHR_shader_subgroup_quad : require
#extension GL_KHR_shader_subgroup_shuffle : require

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

uint WaveLaneCount()
{
    return gl_SubgroupSize;
}

#endif // #ifndef __INTRINSICS_GLSL__