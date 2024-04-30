#include "r_taskGraph.hpp"
#include "renderer/graphics_api/pg_to_vulkan_types.hpp"
#include "renderer/r_globals.hpp"
#include "renderer/r_texture_manager.hpp"
#include <list>

namespace PG::Gfx
{

TGBTextureRef ComputeTaskBuilder::AddTextureOutput( const std::string& name, PixelFormat format, const vec4& clearColor, uint32_t width,
    uint32_t height, uint32_t depth, uint32_t arrayLayers, uint32_t mipLevels )
{
    TGBTextureRef ref = builder->AddTexture( name, format, width, height, depth, arrayLayers, mipLevels, nullptr );
    builder->MarkTextureWrite( ref, taskHandle );
    textures.emplace_back( clearColor, ref, true );
    return ref;
}
TGBTextureRef ComputeTaskBuilder::AddTextureOutput(
    const std::string& name, PixelFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint32_t mipLevels )
{
    TGBTextureRef ref = builder->AddTexture( name, format, width, height, depth, arrayLayers, mipLevels, nullptr );
    builder->MarkTextureWrite( ref, taskHandle );
    textures.emplace_back( vec4( 0 ), ref, false );
    return ref;
}
void ComputeTaskBuilder::AddTextureOutput( TGBTextureRef& texture ) { builder->MarkTextureWrite( texture, taskHandle ); }
void ComputeTaskBuilder::AddTextureInput( TGBTextureRef& texture ) { builder->MarkTextureRead( texture, taskHandle ); }
TGBBufferRef ComputeTaskBuilder::AddBufferOutput(
    const std::string& name, BufferUsage bufferUsage, VmaMemoryUsage memoryUsage, size_t size, uint32_t clearVal )
{
    TGBBufferRef ref = builder->AddBuffer( name, bufferUsage, memoryUsage, size, nullptr );
    builder->MarkBufferWrite( ref, taskHandle );
    buffers.emplace_back( clearVal, ref, true );
    return ref;
}
TGBBufferRef ComputeTaskBuilder::AddBufferOutput(
    const std::string& name, BufferUsage bufferUsage, VmaMemoryUsage memoryUsage, size_t size )
{
    TGBBufferRef ref = builder->AddBuffer( name, bufferUsage, memoryUsage, size, nullptr );
    builder->MarkBufferWrite( ref, taskHandle );
    buffers.emplace_back( 0, ref, false );
    return ref;
}
void ComputeTaskBuilder::AddBufferOutput( TGBBufferRef& buffer ) { builder->MarkBufferWrite( buffer, taskHandle ); }
void ComputeTaskBuilder::AddBufferInput( TGBBufferRef& buffer ) { builder->MarkBufferRead( buffer, taskHandle ); }
void ComputeTaskBuilder::SetFunction( ComputeFunction func ) { function = func; }

TaskGraphBuilder::TaskGraphBuilder() : taskIndex( 0 )
{
    computeTasks.reserve( MAX_TASKS );
    textures.reserve( 256 );
    buffers.reserve( 256 );
    textureAccesses.reserve( 256 );
    bufferAccesses.reserve( 256 );
}

ComputeTaskBuilder* TaskGraphBuilder::AddComputeTask( const std::string& name )
{
    return &computeTasks.emplace_back( this, taskIndex++, name );
}

TGBTextureRef TaskGraphBuilder::RegisterExternalTexture( const std::string& name, PixelFormat format, uint32_t width, uint32_t height,
    uint32_t depth, uint32_t arrayLayers, uint32_t mipLevels, ExtTextureFunc func )
{
    return AddTexture( name, format, width, height, depth, arrayLayers, mipLevels, func );
}

TGBBufferRef TaskGraphBuilder::RegisterExternalBuffer(
    const std::string& name, BufferUsage bufferUsage, VmaMemoryUsage memoryUsage, size_t size, ExtBufferFunc func )
{
    return AddBuffer( name, bufferUsage, memoryUsage, size, func );
}

TGBTextureRef TaskGraphBuilder::AddTexture( const std::string& name, PixelFormat format, uint32_t width, uint32_t height, uint32_t depth,
    uint32_t arrayLayers, uint32_t mipLevels, ExtTextureFunc func )
{
    ResourceIndexHandle index = static_cast<ResourceIndexHandle>( textures.size() );

#if USING( RG_DEBUG )
    textures.emplace_back( name, width, height, depth, arrayLayers, mipLevels, format, func );
#else  // #if USING( RG_DEBUG )
    textures.emplace_back( width, height, depth, arrayLayers, mipLevels, format, func );
#endif // #else // #if USING( RG_DEBUG )
    textureAccesses.emplace_back();

    TGBTextureRef ref;
    ref.index = index;

    return ref;
}

TGBBufferRef TaskGraphBuilder::AddBuffer(
    const std::string& name, BufferUsage bufferUsage, VmaMemoryUsage memoryUsage, size_t size, ExtBufferFunc func )
{
    ResourceIndexHandle index = static_cast<ResourceIndexHandle>( buffers.size() );
#if USING( RG_DEBUG )
    buffers.emplace_back( name, size, bufferUsage, memoryUsage, func );
#else  // #if USING( RG_DEBUG )
    buffers.emplace_back( length, type, format, func );
#endif // #else // #if USING( RG_DEBUG )
    bufferAccesses.emplace_back();

    TGBBufferRef ref;
    ref.index = index;

    return ref;
}

void TaskGraphBuilder::MarkTextureRead( TGBTextureRef ref, TaskHandle task )
{
    textureAccesses[ref.index].firstTask = Min( task.index, textureAccesses[ref.index].firstTask );
    textureAccesses[ref.index].lastTask  = Max( task.index, textureAccesses[ref.index].lastTask );
}
void TaskGraphBuilder::MarkTextureWrite( TGBTextureRef ref, TaskHandle task )
{
    textureAccesses[ref.index].firstTask = Min( task.index, textureAccesses[ref.index].firstTask );
    textureAccesses[ref.index].lastTask  = Max( task.index, textureAccesses[ref.index].lastTask );
}
void TaskGraphBuilder::MarkBufferRead( TGBBufferRef ref, TaskHandle task )
{
    bufferAccesses[ref.index].firstTask = Min( task.index, bufferAccesses[ref.index].firstTask );
    bufferAccesses[ref.index].lastTask  = Max( task.index, bufferAccesses[ref.index].lastTask );
}
void TaskGraphBuilder::MarkBufferWrite( TGBBufferRef ref, TaskHandle task )
{
    bufferAccesses[ref.index].firstTask = Min( task.index, bufferAccesses[ref.index].firstTask );
    bufferAccesses[ref.index].lastTask  = Max( task.index, bufferAccesses[ref.index].lastTask );
}

static void ResolveSizes( TGBTexture& tex, const TaskGraphCompileInfo& info )
{
    tex.width  = ResolveRelativeSize( info.sceneWidth, info.displayWidth, tex.width );
    tex.height = ResolveRelativeSize( info.sceneHeight, info.displayHeight, tex.height );
    if ( tex.mipLevels == AUTO_FULL_MIP_CHAIN() )
    {
        tex.mipLevels = 1 + static_cast<int>( log2f( static_cast<float>( Max( tex.width, tex.height ) ) ) );
    }
}

bool TaskGraph::Compile( TaskGraphBuilder& builder, TaskGraphCompileInfo& compileInfo )
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
    std::vector<ResourceData> resourceDatas;
    resourceDatas.reserve( builder.textures.size() + builder.buffers.size() );
    std::vector<uint16_t> tgbTextureToResourceData( builder.textures.size(), UINT16_MAX );
    textures.resize( builder.textures.size() );
    for ( size_t i = 0; i < builder.textures.size(); ++i )
    {
        TGBTexture& tex = builder.textures[i];
        ResolveSizes( tex, compileInfo );

        Texture& gfxTex        = textures[i];
        TextureCreateInfo desc = {};
        desc.type              = ImageType::TYPE_2D;
        desc.format            = tex.format;
        desc.width             = tex.width;
        desc.height            = tex.height;
        desc.depth             = tex.depth;
        desc.arrayLayers       = tex.arrayLayers;
        desc.mipLevels         = tex.mipLevels;
        // if ( IsSet( lRes.element.type, ResourceType::COLOR_ATTACH ) )
        //     desc.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        // else if ( IsSet( lRes.element.type, ResourceType::DEPTH_ATTACH ) || IsSet( lRes.element.type, ResourceType::STENCIL_ATTACH ) )
        //     desc.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

        gfxTex.m_desc               = desc;
        gfxTex.m_image              = VK_NULL_HANDLE;
        gfxTex.m_imageView          = VK_NULL_HANDLE;
        gfxTex.m_device             = rg.device.GetHandle();
        gfxTex.m_sampler            = GetSampler( desc.sampler );
        gfxTex.m_bindlessArrayIndex = PG_INVALID_TEXTURE_INDEX;

        if ( tex.externalFunc )
            continue;

        tgbTextureToResourceData[i] = (uint16_t)resourceDatas.size();
        ResourceData& data          = resourceDatas.emplace_back();
        data.resType                = ResourceType::TEXTURE;
        data.texRef                 = { (ResourceIndexHandle)i };
        data.firstTask              = builder.textureAccesses[i].firstTask;
        data.lastTask               = builder.textureAccesses[i].lastTask;

        VkImageCreateInfo info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        info.imageType         = VK_IMAGE_TYPE_2D;
        info.extent            = VkExtent3D{ tex.width, tex.height, tex.depth };
        info.mipLevels         = tex.mipLevels;
        info.arrayLayers       = tex.arrayLayers;
        info.format            = PGToVulkanPixelFormat( tex.format );
        info.tiling            = VK_IMAGE_TILING_OPTIMAL;
        info.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
        info.usage =
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.samples     = VK_SAMPLE_COUNT_1_BIT;

        VK_CHECK( vkCreateImage( rg.device.GetHandle(), &info, nullptr, &data.img ) );
        gfxTex.m_image = data.img;
        vkGetImageMemoryRequirements( rg.device.GetHandle(), data.img, &data.memoryReq );
        VkFormatFeatureFlags features = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;
        //      | VK_FORMAT_FEATURE_TRANSFER_DST_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT;
        // if ( isDepth )
        //{
        //    features |= VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
        //}
        VkFormat vkFormat = PGToVulkanPixelFormat( tex.format );
        PG_ASSERT( FormatSupported( vkFormat, features ) );
        gfxTex.m_imageView = CreateImageView( gfxTex.m_image, vkFormat, VK_IMAGE_ASPECT_COLOR_BIT, desc.mipLevels, desc.arrayLayers );
        // tex.m_imageView = CreateImageView(
        //     tex.m_image, vkFormat, isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT, desc.mipLevels, desc.arrayLayers );
        gfxTex.m_bindlessArrayIndex = TextureManager::GetOpenSlot( &gfxTex );
    }

    std::vector<uint16_t> tgbBufferToResourceData( builder.buffers.size(), UINT16_MAX );
    buffers.resize( builder.buffers.size() );
    for ( size_t i = 0; i < builder.buffers.size(); ++i )
    {
        TGBBuffer& buildBuffer  = builder.buffers[i];
        Buffer& gfxBuffer       = buffers[i];
        gfxBuffer.m_bufferUsage = buildBuffer.bufferUsage;
        gfxBuffer.m_memoryUsage = buildBuffer.memoryUsage; // TODO: support others?
        gfxBuffer.m_mappedPtr   = nullptr;
        gfxBuffer.m_size        = buildBuffer.size;
        gfxBuffer.m_handle      = VK_NULL_HANDLE;

        if ( buildBuffer.externalFunc )
            continue;

        tgbBufferToResourceData[i] = (uint16_t)resourceDatas.size();
        ResourceData& data         = resourceDatas.emplace_back();
        data.resType               = ResourceType::BUFFER;
        data.bufRef                = { (ResourceIndexHandle)i };
        data.firstTask             = builder.bufferAccesses[i].firstTask;
        data.lastTask              = builder.bufferAccesses[i].lastTask;

        VkBufferCreateInfo info{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        info.usage = PGToVulkanBufferType( gfxBuffer.m_bufferUsage ) | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        info.size  = buildBuffer.size;
        VK_CHECK( vkCreateBuffer( rg.device, &info, nullptr, &data.buffer ) );
        gfxBuffer.m_handle = data.buffer;
        vkGetBufferMemoryRequirements( rg.device, data.buffer, &data.memoryReq );
    }

    std::sort( resourceDatas.begin(), resourceDatas.end(),
        []( const auto& lhs, const auto& rhs ) { return lhs.memoryReq.size > rhs.memoryReq.size; } );

// assumes alignment is a power of 2
#define ALIGN( offset, alignment ) ( ( offset + alignment - 1 ) & ~( alignment - 1 ) )

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

        Block Overlap( const Block& b ) const
        {
            Block o;
            o.offset        = Max( offset, b.offset );
            size_t lastByte = Min( offset + size, b.offset + b.size );
            o.size          = lastByte < o.offset ? 0 : lastByte - o.offset;
            return o;
        }

        bool CanFit( const ResourceData& res ) const
        {
            size_t alignedOffset = ALIGN( offset, res.memoryReq.alignment );
            size_t padding       = alignedOffset - offset;
            return ( res.memoryReq.size + padding ) <= size;
        }
    };

    struct TimeSlice
    {
        TimeSlice( const ResourceData& res, size_t totalSize ) : TimeSlice( res, totalSize, res.firstTask, res.lastTask, 0 ) {}

        TimeSlice( const ResourceData& res, size_t totalSize, uint16_t inFirstTask, uint16_t inLastTask, size_t offset )
            : firstTask( inFirstTask ), lastTask( inLastTask )
        {
            RG_DEBUG_ONLY( PG_ASSERT( firstTask <= lastTask ) ); // really only 1-off external outputs that should have firstTask==lastTask
            size_t remainingSize = totalSize - res.memoryReq.size;
            if ( remainingSize > 0 )
            {
                localFreeBlocks.emplace_back( 0, totalSize );
                Add( res, offset );
            }
        }

        bool AnyTimeOverlap( const ResourceData& res ) const { return lastTask >= res.firstTask && res.lastTask >= firstTask; }

        void FindFittableRegions(
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

        void Add( const ResourceData& res, size_t alignedOffset )
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
                    RG_DEBUG_ONLY( PG_ASSERT( usedEnd < freeEnd ) );
                    freeBlock = endingFreeBlock;
                }
            }
        }

        std::vector<Block> localFreeBlocks;
        const uint16_t firstTask;
        const uint16_t lastTask;
    };

    struct MemoryBucket
    {
        MemoryBucket( const ResourceData& res )
            : bucketSize( res.memoryReq.size ), initialAlignment( res.memoryReq.alignment ), memoryTypeBits( res.memoryReq.memoryTypeBits )
        {
            occupiedTimeSlices.emplace_back( res, bucketSize );
            if ( res.resType == ResourceType::BUFFER )
                resources.emplace_back( ResHandle( res.bufRef ), 0 );
            else
                resources.emplace_back( ResHandle( res.texRef ), 0 );
        }

        bool AddResource( const ResourceData& res )
        {
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
            RG_DEBUG_ONLY( PG_ASSERT( globalFreeBlocks.size() > 0 ) );

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

            int freeBegin = 0; // track gaps between memory regions
            for ( auto it = occupiedTimeSlices.begin(); it != occupiedTimeSlices.end(); ++it )
            {
                int freeEnd = it->firstTask - 1;
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
                uint16_t start = Max( (uint16_t)freeBegin, res.firstTask );
                TimeSlice endingFreeRegion( res, bucketSize, start, res.lastTask, dstOffset );
                occupiedTimeSlices.emplace_back( endingFreeRegion );
            }
            memoryTypeBits = memoryTypeBits & res.memoryReq.memoryTypeBits;

            return true;
        }

        std::list<TimeSlice> occupiedTimeSlices;
        std::vector<std::pair<ResHandle, size_t>> resources;
        const size_t bucketSize;
        const size_t initialAlignment;
        decltype( VkMemoryRequirements::memoryTypeBits ) memoryTypeBits; // uint32_t
    };

    std::vector<MemoryBucket> buckets;
    std::vector<bool> resHasBucket( resourceDatas.size(), false );
    RG_DEBUG_ONLY( size_t addedResources = 0 );
    for ( size_t resIdx = 0; resIdx < resourceDatas.size(); ++resIdx )
    {
        if ( resHasBucket[resIdx] )
            continue;

        MemoryBucket& currentBucket = buckets.emplace_back( resourceDatas[resIdx] );
        resHasBucket[resIdx]        = true;
        RG_DEBUG_ONLY( ++addedResources );

        for ( size_t i = resIdx + 1; i < resourceDatas.size(); ++i )
        {
            if ( resHasBucket[i] )
                continue;
            if ( currentBucket.AddResource( resourceDatas[i] ) )
            {
                resHasBucket[i] = true;
                RG_DEBUG_ONLY( ++addedResources );
            }
        }
    }
    RG_DEBUG_ONLY( PG_ASSERT( addedResources == resourceDatas.size() ) );

    printf( "" );

    VmaAllocator allocator = rg.device.GetAllocator();
    vmaAllocations.resize( buckets.size() );
    for ( size_t bucketIdx = 0; bucketIdx < buckets.size(); ++bucketIdx )
    {
        const MemoryBucket& bucket       = buckets[bucketIdx];
        VkMemoryRequirements finalMemReq = {};
        finalMemReq.size                 = bucket.bucketSize;
        finalMemReq.alignment            = bucket.initialAlignment;
        finalMemReq.memoryTypeBits       = bucket.memoryTypeBits;

        VmaAllocationCreateInfo allocCreateInfo = {};
        allocCreateInfo.preferredFlags          = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        VmaAllocation& alloc                    = vmaAllocations[bucketIdx];
        VK_CHECK( vmaAllocateMemory( allocator, &finalMemReq, &allocCreateInfo, &alloc, nullptr ) );

        for ( const auto& [resHandle, offset] : bucket.resources )
        {
            if ( resHandle.isTex )
            {
                VkImage img = textures[resHandle.index].m_image;
                VK_CHECK( vmaBindImageMemory2( allocator, alloc, offset, img, nullptr ) );
            }
            else
            {
                VkBuffer buf = buffers[resHandle.index].m_handle;
                VK_CHECK( vmaBindBufferMemory2( allocator, alloc, offset, buf, nullptr ) );
            }
        }
    }

    return true;
}

void TaskGraph::Free()
{
    VmaAllocator allocator = rg.device.GetAllocator();
    for ( VmaAllocation& alloc : vmaAllocations )
    {
        vmaFreeMemory( allocator, alloc );
    }
    for ( Buffer& buf : buffers )
    {
        vmaDestroyBuffer( allocator, buf.m_handle, nullptr );
        // vkDestroyBuffer( rg.device.GetHandle(), buf.m_handle, nullptr );
    }
    for ( Texture& tex : textures )
    {
        vmaDestroyImage( allocator, tex.m_image, nullptr );
        // vkDestroyImage( rg.device.GetHandle(), tex.m_image, nullptr );
        vkDestroyImageView( rg.device.GetHandle(), tex.m_imageView, nullptr );
    }
}

} // namespace PG::Gfx
