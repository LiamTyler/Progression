#include "r_tg_resource_packing.hpp"

namespace PG::Gfx
{

#define ALIGN( offset, alignment ) ( ( offset + alignment - 1 ) & ~( alignment - 1 ) )

Block Block::Overlap( const Block& b ) const
{
    Block o;
    o.offset        = Max( offset, b.offset );
    size_t lastByte = Min( offset + size, b.offset + b.size );
    o.size          = lastByte < o.offset ? 0 : lastByte - o.offset;
    return o;
}

bool Block::CanFit( const ResourceData& res ) const
{
    size_t alignedOffset = ALIGN( offset, res.memoryReq.alignment );
    size_t padding       = alignedOffset - offset;
    return ( res.memoryReq.size + padding ) <= size;
}

TimeSlice::TimeSlice( const ResourceData& res, size_t totalSize ) : TimeSlice( res, totalSize, res.firstTask, res.lastTask, 0 ) {}

TimeSlice::TimeSlice( const ResourceData& res, size_t totalSize, u16 inFirstTask, u16 inLastTask, size_t offset )
    : firstTask( inFirstTask ), lastTask( inLastTask )
{
    // really only 1-off external outputs that should have firstTask==lastTask. Internal, non-depth, should be firstTask < lastTask
    TG_ASSERT( firstTask <= lastTask, "Only depth textures should maybe be used in only 1 pass" );
    size_t remainingSize = totalSize - res.memoryReq.size;
    if ( remainingSize > 0 )
    {
        localFreeBlocks.emplace_back( 0, totalSize );
        Add( res, offset );
    }
}

bool TimeSlice::AnyTimeOverlap( const ResourceData& res ) const { return lastTask >= res.firstTask && res.lastTask >= firstTask; }

void TimeSlice::FindFittableRegions(
    const ResourceData& res, std::vector<Block>& globalFreeBlocks, std::vector<Block>& combinedFreeBlocks ) const
{
    size_t globalIdx = 0;
    size_t localIdx  = 0;
    while ( globalIdx < globalFreeBlocks.size() && localIdx < localFreeBlocks.size() )
    {
        const Block& gBlock = globalFreeBlocks[globalIdx];
        if ( !gBlock.CanFit( res ) )
        {
            ++globalIdx;
            continue;
        }

        const Block& lBlock = localFreeBlocks[localIdx];
        if ( !lBlock.CanFit( res ) )
        {
            ++localIdx;
            continue;
        }
        Block o = gBlock.Overlap( lBlock );
        if ( o.CanFit( res ) )
            combinedFreeBlocks.push_back( o );

        size_t gEnd = gBlock.offset + gBlock.size;
        size_t lEnd = lBlock.offset + lBlock.size;
        if ( gEnd < lEnd )
            ++globalIdx;
        else if ( lEnd < gEnd )
            ++localIdx;
        else
        {
            ++globalIdx;
            ++localIdx;
        }
    }
}

void TimeSlice::Add( const ResourceData& res, size_t alignedOffset )
{
    Block newUsedBlock = { alignedOffset, res.memoryReq.size };
    for ( size_t i = 0; i < localFreeBlocks.size(); ++i )
    {
        Block& freeBlock = localFreeBlocks[i];
        Block overlap    = freeBlock.Overlap( newUsedBlock );
        if ( overlap.size == 0 )
            continue;

        if ( freeBlock.size == newUsedBlock.size )
        {
            localFreeBlocks.erase( localFreeBlocks.begin() + i );
            return;
        }
        size_t usedEnd = newUsedBlock.offset + newUsedBlock.size;
        size_t freeEnd = newUsedBlock.offset + newUsedBlock.size;
        Block beginningFreeBlock{ freeBlock.offset, newUsedBlock.offset - freeBlock.offset };
        Block endingFreeBlock{ usedEnd, freeEnd - usedEnd };
        if ( freeBlock.offset < newUsedBlock.offset )
        {
            freeBlock = beginningFreeBlock;
            if ( usedEnd < freeEnd )
                localFreeBlocks.insert( localFreeBlocks.begin() + i + 1, endingFreeBlock );
        }
        else
        {
            TG_ASSERT( usedEnd < freeEnd );
            freeBlock = endingFreeBlock;
        }
    }
}

MemoryBucket::MemoryBucket( const ResourceData& res )
    : bucketSize( res.memoryReq.size ), initialAlignment( res.memoryReq.alignment ), memoryTypeBits( res.memoryReq.memoryTypeBits )
{
    occupiedTimeSlices.emplace_back( res, bucketSize );
    if ( res.resType == ResourceType::BUFFER )
        resources.emplace_back( ResHandle( res.bufRef ), 0 );
    else
        resources.emplace_back( ResHandle( res.texRef ), 0 );
}

bool MemoryBucket::AddResource( const ResourceData& res )
{
    TG_ASSERT( res.firstTask <= res.lastTask, "Only depth textures should maybe be used in only 1 pass" );
    if ( res.memoryReq.size > bucketSize )
        return false;
    if ( !( memoryTypeBits & res.memoryReq.memoryTypeBits ) )
        return false;
    // since the first region is always the size of the entire allocation
    if ( occupiedTimeSlices.front().AnyTimeOverlap( res ) )
        return false;

    std::vector<Block> globalFreeBlocks, newGlobalFreeBlocks;
    globalFreeBlocks.reserve( 32 );
    newGlobalFreeBlocks.reserve( 32 );
    globalFreeBlocks.emplace_back( 0, bucketSize );
    for ( auto it = occupiedTimeSlices.begin(); it != occupiedTimeSlices.end(); ++it )
    {
        if ( !it->AnyTimeOverlap( res ) )
            continue;
        it->FindFittableRegions( res, globalFreeBlocks, newGlobalFreeBlocks );
        if ( newGlobalFreeBlocks.empty() )
            return false;

        std::swap( globalFreeBlocks, newGlobalFreeBlocks );
        newGlobalFreeBlocks.clear();
    }
    TG_ASSERT( globalFreeBlocks.size() > 0 );

    // use the smallest available free region
    size_t smallestSize = globalFreeBlocks[0].size;
    size_t smallestIdx  = 0;
    for ( size_t i = 1; i < globalFreeBlocks.size(); ++i )
    {
        const Block& b = globalFreeBlocks[i];
        if ( b.size < smallestSize )
        {
            smallestIdx  = i;
            smallestSize = b.size;
        }
    }
    size_t dstOffset = globalFreeBlocks[smallestIdx].offset;
    if ( res.resType == ResourceType::BUFFER )
        resources.emplace_back( ResHandle( res.bufRef ), dstOffset );
    else
        resources.emplace_back( ResHandle( res.texRef ), dstOffset );

    i32 freeBegin = 0; // track gaps between memory regions
    for ( auto it = occupiedTimeSlices.begin(); it != occupiedTimeSlices.end(); ++it )
    {
        i32 freeEnd = it->firstTask - 1;
        if ( freeBegin < freeEnd )
        {
            TimeSlice freeRegion( res, bucketSize, freeBegin, freeEnd, dstOffset );
            occupiedTimeSlices.insert( it, freeRegion );
        }
        if ( it->AnyTimeOverlap( res ) )
            it->Add( res, dstOffset );

        freeBegin = it->lastTask + 1;
    }
    if ( freeBegin <= res.lastTask )
    {
        u16 start = Max( (u16)freeBegin, res.firstTask );
        TimeSlice endingFreeRegion( res, bucketSize, start, res.lastTask, dstOffset );
        occupiedTimeSlices.emplace_back( endingFreeRegion );
    }
    memoryTypeBits = memoryTypeBits & res.memoryReq.memoryTypeBits;

    return true;
}

std::vector<MemoryBucket> PackResources( std::vector<ResourceData>& resourceDatas )
{
    std::sort( resourceDatas.begin(), resourceDatas.end(),
        []( const auto& lhs, const auto& rhs ) { return lhs.memoryReq.size > rhs.memoryReq.size; } );

    std::vector<MemoryBucket> buckets;
    std::vector<bool> resHasBucket( resourceDatas.size(), false );
    TG_DEBUG_ONLY( size_t addedResources = 0 );
    for ( size_t resIdx = 0; resIdx < resourceDatas.size(); ++resIdx )
    {
        if ( resHasBucket[resIdx] )
            continue;

        MemoryBucket& currentBucket = buckets.emplace_back( resourceDatas[resIdx] );
        resHasBucket[resIdx]        = true;
        TG_DEBUG_ONLY( ++addedResources );

        for ( size_t i = resIdx + 1; i < resourceDatas.size(); ++i )
        {
            if ( resHasBucket[i] )
                continue;
            if ( currentBucket.AddResource( resourceDatas[i] ) )
            {
                resHasBucket[i] = true;
                TG_DEBUG_ONLY( ++addedResources );
            }
        }
    }
    TG_ASSERT( addedResources == resourceDatas.size() );

    return buckets;
}

} // namespace PG::Gfx
