#include "r_taskGraph.hpp"
#include "renderer/graphics_api/pg_to_vulkan_types.hpp"
#include "renderer/r_bindless_manager.hpp"
#include "renderer/r_globals.hpp"
#if USING( TG_STATS )
#include "core/time.hpp"
#endif // #if USING( TG_STATS )

namespace PG::Gfx
{

static void ResolveSizes( TGBTexture& tex, const TaskGraph::CompileInfo& info )
{
    tex.width  = ResolveRelativeSize( info.sceneWidth, info.displayWidth, tex.width );
    tex.height = ResolveRelativeSize( info.sceneHeight, info.displayHeight, tex.height );
    if ( tex.mipLevels == AUTO_FULL_MIP_CHAIN() )
    {
        tex.mipLevels = 1 + static_cast<i32>( log2f( static_cast<f32>( Max( tex.width, tex.height ) ) ) );
    }
}

void TaskGraph::Compile_BuildResources( TaskGraphBuilder& builder, CompileInfo& compileInfo, std::vector<ResourceData>& resourceDatas )
{
    resourceDatas.clear();
    resourceDatas.reserve( builder.textures.size() + builder.buffers.size() );

    TG_STAT( m_stats.numTextures = (u32)builder.textures.size() );
    m_textures.resize( builder.textures.size() );
    for ( size_t i = 0; i < builder.textures.size(); ++i )
    {
        TGBTexture& buildTex = builder.textures[i];
        ResolveSizes( buildTex, compileInfo );

        Texture& gfxTex        = m_textures[i];
        TextureCreateInfo desc = {};
        desc.type              = ImageType::TYPE_2D;
        desc.format            = buildTex.format;
        desc.width             = static_cast<u16>( buildTex.width );
        desc.height            = static_cast<u16>( buildTex.height );
        desc.depth             = static_cast<u16>( buildTex.depth );
        desc.arrayLayers       = buildTex.arrayLayers;
        desc.mipLevels         = buildTex.mipLevels;
        desc.usage             = buildTex.usage;

        gfxTex.m_info          = desc;
        gfxTex.m_image         = VK_NULL_HANDLE;
        gfxTex.m_imageView     = VK_NULL_HANDLE;
        gfxTex.m_sampler       = GetSampler( desc.sampler );
        gfxTex.m_bindlessIndex = PG_INVALID_TEXTURE_INDEX;
#if USING( DEBUG_BUILD ) && USING( TG_DEBUG )
        if ( !buildTex.debugName.empty() )
        {
            gfxTex.debugName = (char*)malloc( buildTex.debugName.length() + 1 );
            strcpy( gfxTex.debugName, buildTex.debugName.c_str() );
        }
#endif // #if USING( DEBUG_BUILD ) && USING( TG_DEBUG )

        if ( buildTex.externalFunc )
        {
            m_externalTextures.emplace_back( (TGResourceHandle)i, buildTex.externalFunc );
            continue;
        }

        ResourceData& data = resourceDatas.emplace_back();
        data.resType       = ResourceType::TEXTURE;
        data.texRef        = { (TGResourceHandle)i };
        data.firstTask     = builder.textureLifetimes[i].firstTask;
        data.lastTask      = builder.textureLifetimes[i].lastTask;

        VkImageCreateInfo info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        info.imageType         = VK_IMAGE_TYPE_2D;
        info.extent            = VkExtent3D{ buildTex.width, buildTex.height, buildTex.depth };
        info.mipLevels         = buildTex.mipLevels;
        info.arrayLayers       = buildTex.arrayLayers;
        info.format            = PGToVulkanPixelFormat( buildTex.format );
        info.tiling            = VK_IMAGE_TILING_OPTIMAL;
        info.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
        info.usage             = buildTex.usage;
        info.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;
        info.samples           = VK_SAMPLE_COUNT_1_BIT;

        VK_CHECK( vkCreateImage( rg.device, &info, nullptr, &data.img ) );
        vkGetImageMemoryRequirements( rg.device, data.img, &data.memoryReq );
        gfxTex.m_image = data.img;
        TG_STAT( m_stats.unAliasedTextureMem += data.memoryReq.size );
    }

    TG_STAT( m_stats.numBuffers = (u32)builder.buffers.size() );
    m_buffers.resize( builder.buffers.size() );
    for ( size_t i = 0; i < builder.buffers.size(); ++i )
    {
        TGBBuffer& buildBuffer  = builder.buffers[i];
        Buffer& gfxBuffer       = m_buffers[i];
        gfxBuffer.m_bufferUsage = buildBuffer.bufferUsage | BufferUsage::DEVICE_ADDRESS;
        gfxBuffer.m_memoryUsage = buildBuffer.memoryUsage; // TODO: support others?
        gfxBuffer.m_mappedPtr   = nullptr;
        gfxBuffer.m_size        = buildBuffer.size;
        gfxBuffer.m_handle      = VK_NULL_HANDLE;
#if USING( DEBUG_BUILD ) && USING( TG_DEBUG )
        if ( !buildBuffer.debugName.empty() )
        {
            gfxBuffer.debugName = (char*)malloc( buildBuffer.debugName.length() + 1 );
            strcpy( gfxBuffer.debugName, buildBuffer.debugName.c_str() );
        }
#endif // #if USING( DEBUG_BUILD ) && USING( TG_DEBUG )

        if ( buildBuffer.externalFunc )
        {
            m_externalBuffers.emplace_back( (TGResourceHandle)i, buildBuffer.externalFunc );
            continue;
        }
        TG_STAT( m_stats.unAliasedBufferMem += gfxBuffer.GetSize() );

        ResourceData& data = resourceDatas.emplace_back();
        data.resType       = ResourceType::BUFFER;
        data.bufRef        = { (TGResourceHandle)i };
        data.firstTask     = builder.bufferLifetimes[i].firstTask;
        data.lastTask      = builder.bufferLifetimes[i].lastTask;

        VkBufferCreateInfo info{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        info.usage = PGToVulkanBufferType( gfxBuffer.m_bufferUsage ) | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        info.size  = buildBuffer.size;
        VK_CHECK( vkCreateBuffer( rg.device, &info, nullptr, &data.buffer ) );
#if USING( TG_DEBUG )
        PG_DEBUG_MARKER_SET_BUFFER_NAME( data.buffer, buildBuffer.debugName );
#else  // #if USING( TG_DEBUG )
        PG_DEBUG_MARKER_SET_BUFFER_NAME( data.buffer, "tgBuf_" + std::to_string( i ) );
#endif // #else // #if USING( TG_DEBUG )
        gfxBuffer.m_handle = data.buffer;
        vkGetBufferMemoryRequirements( rg.device, data.buffer, &data.memoryReq );
        TG_STAT( m_stats.unAliasedBufferMem += data.memoryReq.size );
    }
}

static VkImageAspectFlags GetImageAspectFromFormat( PixelFormat format )
{
    VkImageAspectFlags aspect = 0;
    if ( PixelFormatHasDepthFormat( format ) )
        aspect |= VK_IMAGE_ASPECT_DEPTH_BIT;
    if ( PixelFormatHasStencil( format ) )
        aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;

    return ( aspect == 0 ) ? VK_IMAGE_ASPECT_COLOR_BIT : aspect;
}

void TaskGraph::Compile_MemoryAliasing( TaskGraphBuilder& builder, CompileInfo& compileInfo, std::vector<ResourceData>& resourceDatas )
{
    std::vector<MemoryBucket> buckets;
    if ( !compileInfo.mergeResources )
    {
        buckets.reserve( resourceDatas.size() );
        for ( const ResourceData& resData : resourceDatas )
        {
            MemoryBucket& bucket = buckets.emplace_back( resData );
        }
    }
    else
    {
        buckets = PackResources( resourceDatas );
    }

    TG_STAT( auto allocStartTime = Time::GetTimePoint() );
    VmaAllocator allocator = rg.device.GetAllocator();
    m_vmaAllocations.resize( buckets.size() );
    for ( size_t bucketIdx = 0; bucketIdx < buckets.size(); ++bucketIdx )
    {
        const MemoryBucket& bucket       = buckets[bucketIdx];
        VkMemoryRequirements finalMemReq = {};
        finalMemReq.size                 = bucket.bucketSize;
        finalMemReq.alignment            = bucket.initialAlignment;
        finalMemReq.memoryTypeBits       = bucket.memoryTypeBits;
        TG_STAT( m_stats.totalMemoryPostAliasing += bucket.bucketSize );

        VmaAllocationCreateInfo allocCreateInfo = {};
        allocCreateInfo.preferredFlags          = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        VmaAllocation& alloc                    = m_vmaAllocations[bucketIdx];
        VK_CHECK( vmaAllocateMemory( allocator, &finalMemReq, &allocCreateInfo, &alloc, nullptr ) );

        for ( size_t bResIdx = 0; bResIdx < bucket.resources.size(); ++bResIdx )
        {
            const BucketResource& bRes = bucket.resources[bResIdx];
            u16 resIdx                 = bRes.resHandle.index;
            if ( bRes.resHandle.isTex )
            {
                VkImage img = m_textures[resIdx].m_image;
                VK_CHECK( vmaBindImageMemory2( allocator, alloc, bRes.offset, img, nullptr ) );
            }
            else
            {
                Buffer& buf = m_buffers[bRes.resHandle.index];
                VK_CHECK( vmaBindBufferMemory2( allocator, alloc, bRes.offset, buf.m_handle, nullptr ) );

                VkBufferDeviceAddressInfo bdaInfo{};
                bdaInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
                bdaInfo.buffer      = buf;
                buf.m_deviceAddress = vkGetBufferDeviceAddress( rg.device, &bdaInfo );
            }
        }
    }
    TG_STAT( m_stats.resAllocTimeMSec = (f32)Time::GetTimeSince( allocStartTime ) );

#if USING( TG_DEBUG )
    if ( compileInfo.showStats )
    {
        LOG( "Task Graph Memory Aliasing Stats" );
        for ( size_t bucketIdx = 0; bucketIdx < buckets.size(); ++bucketIdx )
        {
            const MemoryBucket& bucket = buckets[bucketIdx];
            TG_STAT( LOG( "Bucket[%zu]: Num Resources: %zu", bucketIdx, bucket.resources.size() ) );

            for ( size_t bResIdx = 0; bResIdx < bucket.resources.size(); ++bResIdx )
            {
                const BucketResource& bRes = bucket.resources[bResIdx];
                u16 resIdx                 = bRes.resHandle.index;
                if ( bRes.resHandle.isTex )
                {
                    VkImage img = m_textures[resIdx].m_image;
                    TG_STAT(
                        LOG( "    Tex %u '%s'. Offset: %zu, Size: %zu", resIdx, m_textures[resIdx].debugName, bRes.offset, bRes.size ) );
                }
                else
                {
                    Buffer& buf = m_buffers[bRes.resHandle.index];
                    TG_STAT(
                        LOG( "    Buf %u '%s'. Offset: %zu, Size: %zu", resIdx, m_buffers[resIdx].debugName, bRes.offset, bRes.size ) );
                }
            }
        }
    }
#endif // #if USING( TG_DEBUG )

    // Vulkan doesn't allow you to make image views until the memory is bound
    for ( size_t i = 0; i < m_textures.size(); ++i )
    {
        TGBTexture& buildTex = builder.textures[i];
        if ( buildTex.externalFunc )
            continue;

        Texture& gfxTex = m_textures[i];

        VkImageAspectFlags aspect = GetImageAspectFromFormat( gfxTex.GetPixelFormat() );
        VkFormat vkFormat         = PGToVulkanPixelFormat( gfxTex.GetPixelFormat() );
        gfxTex.m_imageView        = CreateImageView( gfxTex.m_image, vkFormat, aspect, gfxTex.GetMipLevels(), gfxTex.GetArrayLayers() );
        gfxTex.m_bindlessIndex    = BindlessManager::AddTexture( &gfxTex );

#if USING( TG_DEBUG )
        PG_DEBUG_MARKER_SET_IMAGE_NAME( gfxTex.m_image, buildTex.debugName );
        PG_DEBUG_MARKER_SET_IMAGE_VIEW_NAME( gfxTex.m_imageView, buildTex.debugName );
#else  // #if USING( TG_DEBUG )
        PG_DEBUG_MARKER_SET_IMAGE_NAME( gfxTex.m_image, "tgImg_" + std::to_string( i ) );
        PG_DEBUG_MARKER_SET_IMAGE_VIEW_NAME( gfxTex.m_imageView, "tgImgView_" + std::to_string( i ) );
#endif // #else // #if USING( TG_DEBUG )
    }
}

// if we knew the specific shader stages the buffer was used in, we could a lot more specific with the stages here
static VkPipelineStageFlags2 GetStageFlags( TaskType taskType, BufferUsage bufUsage = BufferUsage::NONE )
{
    switch ( taskType )
    {
    case TaskType::NONE: return VK_PIPELINE_STAGE_2_NONE;
    case TaskType::COMPUTE: return VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    case TaskType::GRAPHICS:
    {
        VkPipelineStageFlags2 ret = 0;
        if ( IsSet( bufUsage, BufferUsage::INDEX ) )
            ret |= VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT;
        if ( IsSet( bufUsage, BufferUsage::VERTEX ) )
            ret |= VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT;
        if ( IsSet( bufUsage, BufferUsage::INDIRECT ) )
            ret |= VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;

        return ret ? ret : VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
    }
    case TaskType::TRANSFER: return VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;
    }

    return VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
}

static VkAccessFlags2 GetAccessFlagsBuffer( TaskType taskType, ResourceState resState, BufferUsage bufUsage = BufferUsage::NONE )
{
    switch ( taskType )
    {
    case TaskType::COMPUTE:
    case TaskType::GRAPHICS:
    {
        if ( IsSet( bufUsage, BufferUsage::INDIRECT ) )
        {
            PG_ASSERT( resState == ResourceState::READ );
            return VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
        }

        if ( resState == ResourceState::READ )
            return VK_ACCESS_2_SHADER_READ_BIT;
        else if ( resState == ResourceState::WRITE )
            return VK_ACCESS_2_SHADER_WRITE_BIT;
        else
            return VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
    }
    case TaskType::TRANSFER:
    {
        TG_ASSERT( resState != ResourceState::READ_WRITE );
        return resState == ResourceState::READ ? VK_ACCESS_2_TRANSFER_READ_BIT : VK_ACCESS_2_TRANSFER_WRITE_BIT;
    }
    case TaskType::PRESENT: return VK_ACCESS_2_NONE;
    }

    return VK_ACCESS_2_NONE;
}

static VkAccessFlags2 GetAccessFlagsTexture( TaskType taskType, ResourceState resState, ResourceType resType = ResourceType::NONE )
{
    switch ( taskType )
    {
    case TaskType::COMPUTE:
    {
        if ( resState == ResourceState::READ )
            return VK_ACCESS_2_SHADER_READ_BIT;
        else if ( resState == ResourceState::WRITE )
            return VK_ACCESS_2_SHADER_WRITE_BIT;
        else
            return VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
    }
    case TaskType::GRAPHICS:
    {
        if ( resState == ResourceState::READ )
        {
            if ( IsSet( resType, ResourceType::COLOR_ATTACH ) )
                return VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;

            if ( IsSet( resType, ResourceType::DEPTH_ATTACH ) || IsSet( resType, ResourceType::STENCIL_ATTACH ) )
                return VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

            return VK_ACCESS_2_SHADER_READ_BIT;
        }
        else if ( resState == ResourceState::WRITE )
        {
            if ( IsSet( resType, ResourceType::COLOR_ATTACH ) )
                return VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;

            if ( IsSet( resType, ResourceType::DEPTH_ATTACH ) || IsSet( resType, ResourceType::STENCIL_ATTACH ) )
                return VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            return VK_ACCESS_2_SHADER_WRITE_BIT;
        }

        return VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
    }
    case TaskType::TRANSFER:
    {
        TG_ASSERT( resState != ResourceState::READ_WRITE );
        return resState == ResourceState::READ ? VK_ACCESS_2_TRANSFER_READ_BIT : VK_ACCESS_2_TRANSFER_WRITE_BIT;
    }
    case TaskType::PRESENT: return VK_ACCESS_2_NONE;
    }

    return VK_ACCESS_2_NONE;
}

static ImageLayout GetImageLayoutFromType( ResourceType type )
{
    if ( IsSet( type, ResourceType::COLOR_ATTACH ) )
        return ImageLayout::COLOR_ATTACHMENT;

    bool depth   = IsSet( type, ResourceType::DEPTH_ATTACH );
    bool stencil = IsSet( type, ResourceType::STENCIL_ATTACH );
    if ( depth && stencil )
        return ImageLayout::DEPTH_STENCIL_ATTACHMENT;
    if ( depth )
        return ImageLayout::DEPTH_ATTACHMENT;
    else if ( stencil )
        return ImageLayout::STENCIL_ATTACHMENT;

    return ImageLayout::GENERAL;
}

static ImageLayout InferImageLayout( TaskType taskType, ResourceState state, PixelFormat pixelFormat )
{
    ImageLayout layout = ImageLayout::GENERAL;
    if ( taskType == TaskType::GRAPHICS )
    {
        if ( state == ResourceState::READ )
        {
            bool hasDepth   = PixelFormatHasDepthFormat( pixelFormat );
            bool hasStencil = PixelFormatHasStencil( pixelFormat );
            if ( hasDepth && hasStencil )
                layout = ImageLayout::DEPTH_STENCIL_READ_ONLY;
            else if ( hasDepth )
                layout = ImageLayout::DEPTH_READ_ONLY;
            else if ( hasStencil )
                layout = ImageLayout::STENCIL_READ_ONLY;
            else
                layout = ImageLayout::SHADER_READ_ONLY;
        }
    }

    return layout;
}

void TaskGraph::Compile_SynchronizationAndTasks( TaskGraphBuilder& builder, CompileInfo& compileInfo )
{
    // create tasks + figure out synchronization barriers needed
    builder.texTracking.resize( builder.textures.size() );
    builder.bufTracking.resize( builder.buffers.size() );

    m_tasks.reserve( builder.tasks.size() );
    for ( u16 taskIndex = 0; taskIndex < (u16)builder.tasks.size(); ++taskIndex )
    {
        TaskBuilder* builderTask = builder.tasks[taskIndex];
        Task* task               = nullptr;

        TaskType taskType = builderTask->taskHandle.type;
        if ( taskType == TaskType::COMPUTE )
        {
            task = Compile_ComputeTask( builderTask, builder, compileInfo );
        }
        else if ( taskType == TaskType::GRAPHICS )
        {
            task = Compile_GraphicsTask( builderTask, builder, compileInfo );
        }
        else if ( taskType == TaskType::TRANSFER )
        {
            task = Compile_TransferTask( builderTask, builder, compileInfo );
        }
        else if ( taskType == TaskType::PRESENT )
        {
            task = Compile_PresentTask( builderTask, builder, compileInfo );
        }

#if USING( PG_GPU_PROFILING ) || USING( TG_DEBUG )
        task->name = builderTask->name;
#endif // #if USING( PG_GPU_PROFILING ) || USING( TG_DEBUG )
        TG_STAT( m_stats.numBarriers_Buffer += (u32)task->bufferBarriers.size() );
        TG_STAT( m_stats.numBarriers_Image += (u32)task->imageBarriers.size() );

        m_tasks.push_back( task );
    }
}

VkBufferMemoryBarrier2 GetBufferBarrier( TGResourceHandle bufIndex, TaskType srcTType, ResourceState srcState, TaskType dstTType,
    ResourceState dstState, BufferUsage dstBufferUsage )
{
    VkBufferMemoryBarrier2 barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2 };
    barrier.srcStageMask  = GetStageFlags( srcTType );
    barrier.srcAccessMask = GetAccessFlagsBuffer( srcTType, srcState );
    barrier.dstStageMask  = GetStageFlags( dstTType, dstBufferUsage );
    barrier.dstAccessMask = GetAccessFlagsBuffer( dstTType, dstState, dstBufferUsage );
    barrier.size          = VK_WHOLE_SIZE;
    barrier.buffer        = reinterpret_cast<VkBuffer>( bufIndex );

    return barrier;
}

VkImageMemoryBarrier2 GetImageBarrier( TGResourceHandle imageIndex, TaskType srcTType, ResourceState srcState, ResourceType srcResType,
    TaskType dstTType, ResourceState dstState, ResourceType dstResType, ImageLayout oldLayout, ImageLayout newLayout )
{
    VkImageMemoryBarrier2 barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
    barrier.srcStageMask  = GetStageFlags( srcTType );
    barrier.srcAccessMask = GetAccessFlagsTexture( srcTType, srcState, srcResType );
    barrier.dstStageMask  = GetStageFlags( dstTType );
    barrier.dstAccessMask = GetAccessFlagsTexture( dstTType, dstState, dstResType );
    barrier.oldLayout     = PGToVulkanImageLayout( oldLayout );
    barrier.newLayout     = PGToVulkanImageLayout( newLayout );
    barrier.image         = reinterpret_cast<VkImage>( imageIndex );

    VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    if ( newLayout == ImageLayout::DEPTH_READ_ONLY || newLayout == ImageLayout::DEPTH_ATTACHMENT )
        aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
    else if ( newLayout == ImageLayout::STENCIL_READ_ONLY || newLayout == ImageLayout::STENCIL_ATTACHMENT )
        aspect = VK_IMAGE_ASPECT_STENCIL_BIT;
    else if ( newLayout == ImageLayout::DEPTH_STENCIL_READ_ONLY || newLayout == ImageLayout::DEPTH_STENCIL_ATTACHMENT ||
              newLayout == ImageLayout::DEPTH_READ_ONLY_STENCIL_ATTACHMENT || newLayout == ImageLayout::DEPTH_ATTACHMENT_STENCIL_READ_ONLY )
        aspect = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    barrier.subresourceRange = ImageSubresourceRange( aspect );

    return barrier;
}

void TaskGraph::Compile_PipelineTask( PipelineTask* pTask, PipelineTaskBuilder* bpTask, TaskGraphBuilder& builder )
{
    u16 taskIndex     = bpTask->taskHandle.index;
    TaskType taskType = bpTask->taskHandle.type;
    for ( const TGBBufferInfo& bufInfo : bpTask->buffers )
    {
        TGResourceHandle index          = bufInfo.ref.index;
        const TGBBuffer& buildBuffer    = builder.buffers[index];
        ResourceTrackingInfo& trackInfo = builder.bufTracking[index];
        ResourceType resType            = ResourceType::BUFFER;
        if ( bufInfo.isCleared )
        {
            TG_ASSERT( trackInfo.prevTask == NO_TASK || ( trackInfo.prevTask != NO_TASK && trackInfo.prevState == ResourceState::READ ),
                "Shouldn't be clearing a texture you just wrote to" );
            TG_ASSERT( bufInfo.state == ResourceState::WRITE, "Don't think reading from a just-cleared resource is intended" );
            pTask->bufferClears.emplace_back( index, bufInfo.clearVal );

            VkBufferMemoryBarrier2 barrier =
                GetBufferBarrier( index, TaskType::TRANSFER, ResourceState::WRITE, taskType, bufInfo.state, bufInfo.usageInTask );
            pTask->bufferBarriers.push_back( barrier );
        }
        else if ( trackInfo.prevTask != NO_TASK && trackInfo.prevState != ResourceState::READ )
        {
            // barrier to wait for any previous writes to be complete
            VkBufferMemoryBarrier2 barrier =
                GetBufferBarrier( index, trackInfo.prevTaskType, trackInfo.prevState, taskType, bufInfo.state, buildBuffer.bufferUsage );
            pTask->bufferBarriers.push_back( barrier );
        }

        if ( bufInfo.state == ResourceState::READ || bufInfo.state == ResourceState::READ_WRITE )
            pTask->inputBuffers.push_back( index );
        if ( bufInfo.state == ResourceState::WRITE || bufInfo.state == ResourceState::READ_WRITE )
            pTask->outputBuffers.push_back( index );

        trackInfo = ResourceTrackingInfo( taskIndex, taskType, bufInfo.state, resType );
    }

    for ( const TGBTextureInfo& texInfo : bpTask->textures )
    {
        TGResourceHandle index          = texInfo.ref.index;
        const TGBTexture& buildTexture  = builder.textures[index];
        ResourceTrackingInfo& trackInfo = builder.texTracking[index];
        ResourceType resType            = ResourceType::TEXTURE;
        ImageLayout desiredLayout       = InferImageLayout( taskType, texInfo.state, buildTexture.format );

        if ( texInfo.isCleared )
        {
            TG_ASSERT( trackInfo.prevTask == NO_TASK || ( trackInfo.prevTask != NO_TASK && trackInfo.prevState == ResourceState::READ ),
                "Shouldn't be clearing a texture you just wrote to" );
            TG_ASSERT( texInfo.state == ResourceState::WRITE, "Don't think reading from a just-cleared resource is intended" );
            pTask->textureClears.emplace_back( index, texInfo.clearColor );

            // image layout transition, if necessary
            if ( trackInfo.currLayout != ImageLayout::TRANSFER_DST )
            {
                VkImageMemoryBarrier2 barrier = GetImageBarrier( index, TaskType::NONE, ResourceState::UNUSED, trackInfo.prevResType,
                    TaskType::TRANSFER, ResourceState::WRITE, resType, trackInfo.currLayout, ImageLayout::TRANSFER_DST );
                pTask->imageBarriersPreClears.push_back( barrier );
            }

            // barrier to wait for the clear to be done before running the actual task
            VkImageMemoryBarrier2 barrier = GetImageBarrier( index, TaskType::TRANSFER, ResourceState::WRITE, trackInfo.prevResType,
                taskType, ResourceState::WRITE, resType, ImageLayout::TRANSFER_DST, desiredLayout );
            pTask->imageBarriers.push_back( barrier );
        }
        else if ( trackInfo.prevTask != NO_TASK && trackInfo.prevState != ResourceState::READ )
        {
            // barrier to wait for any previous writes to be complete
            VkImageMemoryBarrier2 barrier = GetImageBarrier( index, trackInfo.prevTaskType, trackInfo.prevState, trackInfo.prevResType,
                taskType, texInfo.state, resType, trackInfo.currLayout, desiredLayout );
            pTask->imageBarriers.push_back( barrier );
        }
        else if ( trackInfo.prevTask == NO_TASK )
        {
            TG_ASSERT( trackInfo.currLayout == ImageLayout::UNDEFINED, "Should be the first use of this texture" );
            VkImageMemoryBarrier2 barrier = GetImageBarrier( index, TaskType::NONE, ResourceState::UNUSED, trackInfo.prevResType, taskType,
                texInfo.state, resType, trackInfo.currLayout, desiredLayout );
            pTask->imageBarriers.push_back( barrier );
        }

        if ( texInfo.state == ResourceState::READ || texInfo.state == ResourceState::READ_WRITE )
            pTask->inputTextures.push_back( index );
        if ( texInfo.state == ResourceState::WRITE || texInfo.state == ResourceState::READ_WRITE )
            pTask->outputTextures.push_back( index );

        trackInfo = ResourceTrackingInfo( ImageLayout::GENERAL, taskIndex, taskType, texInfo.state, resType );
    }

    TG_STAT( m_stats.numBarriers_Image += (u32)pTask->imageBarriersPreClears.size() );
}

Task* TaskGraph::Compile_ComputeTask( TaskBuilder* builderTask, TaskGraphBuilder& builder, CompileInfo& compileInfo )
{
    TG_STAT( m_stats.numComputeTasks++ );
    ComputeTaskBuilder* bcTask = static_cast<ComputeTaskBuilder*>( builderTask );
    ComputeTask* cTask         = new ComputeTask;
    cTask->function            = bcTask->function;
    TG_ASSERT( cTask->function, "Compute tasks are required to have a function!" );
    Compile_PipelineTask( cTask, bcTask, builder );

    return cTask;
}

Task* TaskGraph::Compile_GraphicsTask( TaskBuilder* builderTask, TaskGraphBuilder& builder, CompileInfo& compileInfo )
{
    TG_STAT( m_stats.numGraphicsTasks++ );
    GraphicsTaskBuilder* bgTask = static_cast<GraphicsTaskBuilder*>( builderTask );
    GraphicsTask* gTask         = new GraphicsTask;
    gTask->function             = bgTask->function;
    Compile_PipelineTask( gTask, bgTask, builder );

    u16 taskIndex       = builderTask->taskHandle.index;
    TaskType taskType   = builderTask->taskHandle.type;
    u32 minAttachWidth  = ~0u;
    u32 minAttachHeight = ~0u;
    for ( const TGBAttachmentInfo& bAttachInfo : bgTask->attachments )
    {
        TGResourceHandle texHandle      = bAttachInfo.ref.index;
        const Texture& tex              = m_textures[texHandle];
        ResourceTrackingInfo& trackInfo = builder.texTracking[texHandle];
        bool isDepthAttach              = PixelFormatHasDepthFormat( tex.GetPixelFormat() );
        ResourceType resType            = bAttachInfo.type;

        ImageLayout desiredLayout = GetImageLayoutFromType( bAttachInfo.type );
        if ( trackInfo.currLayout != desiredLayout )
        {
            VkImageMemoryBarrier2 barrier = GetImageBarrier( texHandle, trackInfo.prevTaskType, trackInfo.prevState, trackInfo.prevResType,
                taskType, ResourceState::WRITE, resType, trackInfo.currLayout, desiredLayout );
            gTask->imageBarriers.push_back( barrier );
        }

        VkRenderingAttachmentInfo* vkAttach;
        if ( isDepthAttach )
        {
            TG_ASSERT( !gTask->depthAttach, "can't have multiple depth attachments" );
            gTask->depthAttach = new VkRenderingAttachmentInfo;
            vkAttach           = gTask->depthAttach;
        }
        else
        {
            vkAttach = &gTask->colorAttachments.emplace_back();
        }
        *vkAttach             = VkRenderingAttachmentInfo{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
        vkAttach->imageView   = reinterpret_cast<VkImageView>( texHandle );
        vkAttach->imageLayout = PGToVulkanImageLayout( desiredLayout );
        vkAttach->storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
        if ( bAttachInfo.isCleared )
            vkAttach->loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        else if ( trackInfo.currLayout == ImageLayout::UNDEFINED )
            vkAttach->loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        else
            vkAttach->loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

        TG_ASSERT( !PixelFormatHasStencil( tex.GetPixelFormat() ), "handle stencil clears" );
        if ( isDepthAttach )
            vkAttach->clearValue.depthStencil.depth = bAttachInfo.clearColor.x;
        else
            memcpy( vkAttach->clearValue.color.float32, &bAttachInfo.clearColor.x, sizeof( vec4 ) );

        minAttachWidth  = Min( minAttachWidth, m_textures[texHandle].GetWidth() );
        minAttachHeight = Min( minAttachHeight, m_textures[texHandle].GetHeight() );

        trackInfo = ResourceTrackingInfo( desiredLayout, taskIndex, taskType, ResourceState::WRITE, resType );
    }

    gTask->renderingInfo.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
    gTask->renderingInfo.pNext                = nullptr;
    gTask->renderingInfo.flags                = 0;
    gTask->renderingInfo.renderArea           = { 0, 0, minAttachWidth, minAttachHeight };
    gTask->renderingInfo.layerCount           = 1;
    gTask->renderingInfo.viewMask             = 0;
    gTask->renderingInfo.colorAttachmentCount = (u32)gTask->colorAttachments.size();
    gTask->renderingInfo.pColorAttachments    = gTask->colorAttachments.data();
    gTask->renderingInfo.pDepthAttachment     = gTask->depthAttach;
    gTask->renderingInfo.pStencilAttachment   = nullptr;

    return gTask;
}

Task* TaskGraph::Compile_TransferTask( TaskBuilder* builderTask, TaskGraphBuilder& builder, CompileInfo& compileInfo )
{
    TG_STAT( m_stats.numTransferTasks++ );
    TransferTaskBuilder* btTask = static_cast<TransferTaskBuilder*>( builderTask );
    TransferTask* tTask         = new TransferTask;

    u16 taskIndex     = btTask->taskHandle.index;
    TaskType taskType = btTask->taskHandle.type;

    for ( const TGBTextureTransfer& transfer : btTask->textureBlits )
    {
        ResourceTrackingInfo& srcTrackInfo = builder.texTracking[transfer.src.index];
        ResourceTrackingInfo& dstTrackInfo = builder.texTracking[transfer.dst.index];
        TG_ASSERT( srcTrackInfo.prevTask != NO_TASK && srcTrackInfo.currLayout != ImageLayout::UNDEFINED,
            "Need valid data to blit to another image!" );

        ResourceType resType             = ResourceType::TEXTURE;
        VkImageMemoryBarrier2 srcBarrier = GetImageBarrier( transfer.src.index, srcTrackInfo.prevTaskType, srcTrackInfo.prevState,
            srcTrackInfo.prevResType, taskType, ResourceState::READ, resType, srcTrackInfo.currLayout, ImageLayout::TRANSFER_SRC );
        tTask->imageBarriers.push_back( srcBarrier );

        VkImageMemoryBarrier2 dstBarrier = GetImageBarrier( transfer.dst.index, dstTrackInfo.prevTaskType, dstTrackInfo.prevState,
            dstTrackInfo.prevResType, taskType, ResourceState::WRITE, resType, dstTrackInfo.currLayout, ImageLayout::TRANSFER_DST );
        tTask->imageBarriers.push_back( dstBarrier );

        srcTrackInfo = ResourceTrackingInfo( ImageLayout::TRANSFER_SRC, taskIndex, taskType, ResourceState::READ, resType );
        dstTrackInfo = ResourceTrackingInfo( ImageLayout::TRANSFER_DST, taskIndex, taskType, ResourceState::WRITE, resType );

        tTask->textureBlits.emplace_back( transfer.dst.index, transfer.src.index );
    }

    return tTask;
}

Task* TaskGraph::Compile_PresentTask( TaskBuilder* builderTask, TaskGraphBuilder& builder, CompileInfo& compileInfo )
{
    PresentTaskBuilder* bpTask = static_cast<PresentTaskBuilder*>( builderTask );
    PresentTask* pTask         = new PresentTask;
    u16 taskIndex              = bpTask->taskHandle.index;
    TaskType taskType          = bpTask->taskHandle.type;

    ResourceTrackingInfo& trackInfo = builder.texTracking[bpTask->presentationTex.index];
    TG_ASSERT( trackInfo.prevTask != NO_TASK && trackInfo.currLayout != ImageLayout::UNDEFINED,
        "Need to have written data to the image you're about to present!" );

    VkImageMemoryBarrier2 barrier = GetImageBarrier( bpTask->presentationTex.index, trackInfo.prevTaskType, trackInfo.prevState,
        trackInfo.prevResType, taskType, ResourceState::READ, ResourceType::TEXTURE, trackInfo.currLayout, ImageLayout::PRESENT_SRC_KHR );
    pTask->imageBarriers.push_back( barrier );

    trackInfo = ResourceTrackingInfo( ImageLayout::PRESENT_SRC_KHR, taskIndex, taskType, ResourceState::READ, ResourceType::TEXTURE );

    return pTask;
}

bool TaskGraph::Compile( TaskGraphBuilder& builder, CompileInfo& compileInfo )
{
    TG_STAT( m_stats = {} );
    TG_STAT( auto compileStartTime = Time::GetTimePoint() );

    std::vector<ResourceData> resourceDatas;
    Compile_BuildResources( builder, compileInfo, resourceDatas );
    Compile_MemoryAliasing( builder, compileInfo, resourceDatas );
    Compile_SynchronizationAndTasks( builder, compileInfo );

    TG_STAT( m_stats.compileTimeMSec = (f32)Time::GetTimeSince( compileStartTime ) );
    TG_STAT( if ( compileInfo.showStats ) DisplayStats() );

    return true;
}

void TaskGraph::Free()
{
    for ( Task* task : m_tasks )
        delete task;

    // remove the external buffers and textures first
    for ( const auto& [resHandle, _] : m_externalBuffers )
    {
        m_buffers[resHandle].m_handle = VK_NULL_HANDLE;
    }
    for ( const auto& [resHandle, _] : m_externalTextures )
    {
        m_textures[resHandle].m_image = VK_NULL_HANDLE;
    }

    VmaAllocator allocator = rg.device.GetAllocator();
    for ( VmaAllocation& alloc : m_vmaAllocations )
    {
        vmaFreeMemory( allocator, alloc );
    }
    for ( Buffer& buf : m_buffers )
    {
        if ( buf.m_handle != VK_NULL_HANDLE )
            vmaDestroyBuffer( allocator, buf.m_handle, nullptr );
    }
    for ( Texture& tex : m_textures )
    {
        if ( tex.m_image != VK_NULL_HANDLE )
        {
            vmaDestroyImage( allocator, tex.m_image, nullptr );
            vkDestroyImageView( rg.device, tex.m_imageView, nullptr );
        }
    }

    m_tasks.clear();
    m_buffers.clear();
    m_textures.clear();
    m_vmaAllocations.clear();
    m_externalBuffers.clear();
    m_externalTextures.clear();
}

void TaskGraph::DisplayStats()
{
#if USING( TG_STATS )
    f32 toMB      = 1.0f / ( 1024 * 1024 );
    const auto& s = m_stats;
    LOG( "Task Graph Stats:" );
    LOG( "    Compiled in %.2fms", s.compileTimeMSec );
    LOG( "        Res allocation took %.2fms (%.0f%% of total)", s.resAllocTimeMSec, 100 * s.resAllocTimeMSec / s.compileTimeMSec );
    LOG( "    Num Tasks: %u", s.numComputeTasks + s.numGraphicsTasks + s.numTransferTasks );
    LOG( "        Compute: %u, Graphics: %u, Transfer: %u", s.numComputeTasks, s.numGraphicsTasks, s.numTransferTasks );
    LOG( "    Num Pipeline Barriers: %u", s.numBarriers_Buffer + s.numBarriers_Image + s.numBarriers_Global );
    LOG( "        Buffer: %u, Image: %u, Global: %u", s.numBarriers_Buffer, s.numBarriers_Image, s.numBarriers_Global );
    LOG( "    Textures: %u (%.3f MB unaliased)", s.numTextures, s.unAliasedTextureMem * toMB );
    LOG( "    Buffers: %u (%.3f MB unaliased)", s.numBuffers, s.unAliasedBufferMem * toMB );
    size_t preAliasMem = s.unAliasedTextureMem + s.unAliasedBufferMem;
    LOG( "    Total memory post-aliasing: %.3f MB (aliasing saved %.3f MB)", s.totalMemoryPostAliasing * toMB,
        ( preAliasMem - s.totalMemoryPostAliasing ) * toMB );
    LOG( "" );
#endif // #if USING( TG_STATS )
}

#if USING( TG_DEBUG )
void TaskGraph::Print()
{
    LOG( "TaskGraph" );
    LOG( "------------------------------------------------------------" );
    LOG( "Tasks: %zu", m_tasks.size() );

    for ( size_t taskIdx = 0; taskIdx < m_tasks.size(); ++taskIdx )
    {
        Task* task = m_tasks[taskIdx];
        LOG( "  Task[%zu]: '%s'", taskIdx, task->name.c_str() );
        task->Print( this );
    }
}
#endif // #if USING( TG_DEBUG )

void TaskGraph::Execute( TGExecuteData& data )
{
    PGP_ZONE_SCOPEDN( "TaskGraph::Execute" );
    data.taskGraph = this;
    for ( auto& [bufHandle, extBufFunc] : m_externalBuffers )
    {
        m_buffers[bufHandle] = extBufFunc();
    }
    for ( auto& [texHandle, extTexFunc] : m_externalTextures )
    {
        m_textures[texHandle] = extTexFunc();
    }

    for ( Task* task : m_tasks )
    {
        PG_DEBUG_MARKER_BEGIN_REGION_CMDBUF( *data.cmdBuf, task->name );
        PG_PROFILE_GPU_START( *data.cmdBuf, tgTask, task->name );
        task->Execute( &data );
        PG_PROFILE_GPU_END( *data.cmdBuf, tgTask );
        PG_DEBUG_MARKER_END_REGION_CMDBUF( *data.cmdBuf );
    }
}

Buffer* TaskGraph::GetBuffer( TGResourceHandle handle ) { return &m_buffers[handle]; }
Texture* TaskGraph::GetTexture( TGResourceHandle handle ) { return &m_textures[handle]; }

} // namespace PG::Gfx
