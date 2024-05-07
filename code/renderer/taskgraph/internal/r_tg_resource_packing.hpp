#pragma once

#include "r_tg_builder.hpp"
#include <list>

namespace PG::Gfx
{

struct ResourceData
{
    VkMemoryRequirements memoryReq;
    union
    {
        struct
        {
            VkImage img;
            TGBTextureRef texRef;
        };
        struct
        {
            VkBuffer buffer;
            TGBBufferRef bufRef;
        };
    };
    uint16_t firstTask;
    uint16_t lastTask;
    ResourceType resType;
};

struct ResHandle
{
    uint16_t index : 15;
    uint16_t isTex : 1;

    ResHandle( TGBBufferRef buf ) : index( buf.index ), isTex( 0 ) {}
    ResHandle( TGBTextureRef tex ) : index( tex.index ), isTex( 1 ) {}
};

struct Block
{
    size_t offset;
    size_t size;

    Block Overlap( const Block& b ) const;
    bool CanFit( const ResourceData& res ) const;
};

struct TimeSlice
{
    TimeSlice( const ResourceData& res, size_t totalSize );
    TimeSlice( const ResourceData& res, size_t totalSize, uint16_t inFirstTask, uint16_t inLastTask, size_t offset );

    bool AnyTimeOverlap( const ResourceData& res ) const;
    void FindFittableRegions( const ResourceData& res, std::vector<Block>& globalFreeBlocks, std::vector<Block>& combinedFreeBlocks ) const;
    void Add( const ResourceData& res, size_t alignedOffset );

    std::vector<Block> localFreeBlocks;
    const uint16_t firstTask;
    const uint16_t lastTask;
};

struct MemoryBucket
{
    MemoryBucket( const ResourceData& res );

    bool AddResource( const ResourceData& res );

    std::list<TimeSlice> occupiedTimeSlices;
    std::vector<std::pair<ResHandle, size_t>> resources;
    const size_t bucketSize;
    const size_t initialAlignment;
    decltype( VkMemoryRequirements::memoryTypeBits ) memoryTypeBits; // uint32_t
};

std::vector<MemoryBucket> PackResources( std::vector<ResourceData>& resourceDatas );

} // namespace PG::Gfx
