#include "r_taskGraph.hpp"
#include "renderer/graphics_api/pg_to_vulkan_types.hpp"
#include "renderer/r_globals.hpp"
#include "renderer/r_texture_manager.hpp"

namespace PG::Gfx
{

static void ResolveSizes( TGBTexture& tex, const TaskGraph::CompileInfo& info )
{
    tex.width  = ResolveRelativeSize( info.sceneWidth, info.displayWidth, tex.width );
    tex.height = ResolveRelativeSize( info.sceneHeight, info.displayHeight, tex.height );
    if ( tex.mipLevels == AUTO_FULL_MIP_CHAIN() )
    {
        tex.mipLevels = 1 + static_cast<int>( log2f( static_cast<float>( Max( tex.width, tex.height ) ) ) );
    }
}

void TaskGraph::Compile_BuildResources( TaskGraphBuilder& builder, CompileInfo& compileInfo, std::vector<ResourceData>& resourceDatas )
{
    resourceDatas.clear();
    resourceDatas.reserve( builder.textures.size() + builder.buffers.size() );

    textures.resize( builder.textures.size() );
    for ( size_t i = 0; i < builder.textures.size(); ++i )
    {
        TGBTexture& buildTex = builder.textures[i];
        ResolveSizes( buildTex, compileInfo );

        Texture& gfxTex        = textures[i];
        TextureCreateInfo desc = {};
        desc.type              = ImageType::TYPE_2D;
        desc.format            = buildTex.format;
        desc.width             = buildTex.width;
        desc.height            = buildTex.height;
        desc.depth             = buildTex.depth;
        desc.arrayLayers       = buildTex.arrayLayers;
        desc.mipLevels         = buildTex.mipLevels;
        desc.usage             = buildTex.usage;

        gfxTex.m_info               = desc;
        gfxTex.m_image              = VK_NULL_HANDLE;
        gfxTex.m_imageView          = VK_NULL_HANDLE;
        gfxTex.m_sampler            = GetSampler( desc.sampler );
        gfxTex.m_bindlessArrayIndex = PG_INVALID_TEXTURE_INDEX;

        if ( buildTex.externalFunc )
        {
            externalTextures.emplace_back( (TGResourceHandle)i, buildTex.externalFunc );
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
        info.usage =
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.samples     = VK_SAMPLE_COUNT_1_BIT;

        VK_CHECK( vkCreateImage( rg.device, &info, nullptr, &data.img ) );
        vkGetImageMemoryRequirements( rg.device, data.img, &data.memoryReq );
        gfxTex.m_image = data.img;
    }

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
        {
            externalBuffers.emplace_back( (TGResourceHandle)i, buildBuffer.externalFunc );
            continue;
        }

        ResourceData& data = resourceDatas.emplace_back();
        data.resType       = ResourceType::BUFFER;
        data.bufRef        = { (TGResourceHandle)i };
        data.firstTask     = builder.textureLifetimes[i].firstTask;
        data.lastTask      = builder.textureLifetimes[i].lastTask;

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

#if USING( TG_DEBUG )
        PG_DEBUG_MARKER_SET_BUFFER_NAME( gfxBuffer, buildBuffer.debugName );
#endif // #if USING( TG_DEBUG )
    }
}

void TaskGraph::Compile_MemoryAliasing( TaskGraphBuilder& builder, CompileInfo& compileInfo, std::vector<ResourceData>& resourceDatas )
{
    std::vector<MemoryBucket> buckets = PackResources( resourceDatas );

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

    // Vulkan doesn't allow you to make image views until the memory is bound
    for ( size_t i = 0; i < textures.size(); ++i )
    {
        TGBTexture& buildTex = builder.textures[i];
        if ( buildTex.externalFunc )
            continue;

        Texture& gfxTex = textures[i];

        VkFormatFeatureFlags features = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;
        VkFormat vkFormat             = PGToVulkanPixelFormat( gfxTex.GetPixelFormat() );
        PG_ASSERT( FormatSupported( vkFormat, features ) );
        gfxTex.m_imageView =
            CreateImageView( gfxTex.m_image, vkFormat, VK_IMAGE_ASPECT_COLOR_BIT, gfxTex.GetMipLevels(), gfxTex.GetArrayLayers() );

        gfxTex.m_bindlessArrayIndex = TextureManager::AddTexture( gfxTex.m_imageView );

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
static VkPipelineStageFlags2 GetGeneralStageFlags( TaskType taskType )
{
    switch ( taskType )
    {
    case TaskType::NONE: return VK_PIPELINE_STAGE_2_NONE;
    case TaskType::COMPUTE: return VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    case TaskType::GRAPHICS: return VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
    case TaskType::TRANSFER: return VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;
    }

    return VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
}

static VkPipelineStageFlags2 GetSrcStageFlags( TaskType taskType, BufferUsage bufUsage )
{
    switch ( taskType )
    {
    case TaskType::NONE: return VK_PIPELINE_STAGE_2_NONE;
    case TaskType::COMPUTE: return VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    case TaskType::GRAPHICS: return VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
    case TaskType::TRANSFER: return VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;
    }

    return VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
}

static VkPipelineStageFlags2 GetDstStageFlags( TaskType taskType, BufferUsage bufUsage )
{
    TG_ASSERT( taskType != TaskType::NONE );
    switch ( taskType )
    {
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

static VkAccessFlags2 GetAccessFlags( TaskType taskType, ResourceState resState, ResourceType resType = ResourceType::NONE )
{
    switch ( taskType )
    {
    case TaskType::COMPUTE:
    {
        if ( resState == ResourceState::READ )
            return VK_ACCESS_2_SHADER_READ_BIT;
        else if ( resState == ResourceState::READ )
            return VK_ACCESS_2_SHADER_WRITE_BIT;
        else
            return VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
    }
    case TaskType::GRAPHICS:
    {
        TG_ASSERT( resState == ResourceState::WRITE );
        return VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
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

void TaskGraph::Compile_SynchronizationAndTasks( TaskGraphBuilder& builder, CompileInfo& compileInfo )
{
    // create tasks + figure out synchronization barriers needed
    static constexpr uint16_t NO_TASK = UINT16_MAX;
    struct ResourceTrackingInfo
    {
        ImageLayout currLayout = ImageLayout::UNDEFINED;
        uint16_t prevTask      = NO_TASK;
        TaskType prevTaskType  = TaskType::NONE;
        ResourceState prevState;
    };
    std::vector<ResourceTrackingInfo> texTracking( builder.textures.size() );
    std::vector<ResourceTrackingInfo> bufTracking( builder.buffers.size() );

    tasks.reserve( builder.tasks.size() );
    for ( uint16_t taskIndex = 0; taskIndex < (uint16_t)builder.tasks.size(); ++taskIndex )
    {
        TaskBuilder* builderTask = builder.tasks[taskIndex];
        Task* task               = nullptr;

        TaskType taskType = builderTask->taskHandle.type;
        if ( taskType == TaskType::COMPUTE )
        {
            ComputeTaskBuilder* bcTask = static_cast<ComputeTaskBuilder*>( builderTask );
            ComputeTask* cTask         = new ComputeTask;
            task                       = cTask;
            cTask->function            = bcTask->function;
            TG_ASSERT( cTask->function, "Compute tasks are required to have a function!" );

            for ( const TGBBufferInfo& bufInfo : bcTask->buffers )
            {
                TGResourceHandle index          = bufInfo.ref.index;
                const TGBBuffer& buildBuffer    = builder.buffers[index];
                ResourceTrackingInfo& trackInfo = bufTracking[index];
                if ( bufInfo.isCleared )
                {
                    TG_ASSERT(
                        trackInfo.prevTask == NO_TASK || ( trackInfo.prevTask != NO_TASK && trackInfo.prevState == ResourceState::READ ),
                        "Shouldn't be clearing a texture you just wrote to" );
                    TG_ASSERT( bufInfo.state == ResourceState::WRITE, "Don't think reading from a just-cleared resource is intended" );
                    cTask->bufferClears.emplace_back( index, bufInfo.clearVal );

                    // barrier to wait for the clear to be done before running the actual task
                    VkBufferMemoryBarrier2 barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2 };
                    barrier.srcStageMask  = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;
                    barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                    barrier.dstStageMask  = GetDstStageFlags( taskType, buildBuffer.bufferUsage );
                    barrier.dstAccessMask = GetAccessFlags( taskType, bufInfo.state );
                    barrier.size          = VK_WHOLE_SIZE;
                    barrier.buffer        = reinterpret_cast<VkBuffer>( index );
                    task->bufferBarriers.push_back( barrier );
                }
                else if ( trackInfo.prevTask != NO_TASK && trackInfo.prevState != ResourceState::READ )
                {
                    // barrier to wait for any previous writes to be complete
                    VkBufferMemoryBarrier2 barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2 };
                    barrier.srcStageMask  = GetGeneralStageFlags( trackInfo.prevTaskType );
                    barrier.srcAccessMask = GetAccessFlags( trackInfo.prevTaskType, trackInfo.prevState );
                    barrier.dstStageMask  = GetDstStageFlags( taskType, buildBuffer.bufferUsage );
                    barrier.dstAccessMask = GetAccessFlags( taskType, bufInfo.state );
                    barrier.size          = VK_WHOLE_SIZE;
                    barrier.buffer        = reinterpret_cast<VkBuffer>( index );
                    task->bufferBarriers.push_back( barrier );
                }

                if ( bufInfo.state == ResourceState::READ || bufInfo.state == ResourceState::READ_WRITE )
                    cTask->inputBuffers.push_back( index );
                if ( bufInfo.state == ResourceState::WRITE || bufInfo.state == ResourceState::READ_WRITE )
                    cTask->outputBuffers.push_back( index );

                trackInfo.prevTask     = taskIndex;
                trackInfo.prevState    = bufInfo.state;
                trackInfo.prevTaskType = taskType;
            }

            for ( const TGBTextureInfo& texInfo : bcTask->textures )
            {
                TGResourceHandle index          = texInfo.ref.index;
                const TGBTexture& buildTexture  = builder.textures[index];
                ResourceTrackingInfo& trackInfo = texTracking[index];
                if ( texInfo.isCleared )
                {
                    TG_ASSERT(
                        trackInfo.prevTask == NO_TASK || ( trackInfo.prevTask != NO_TASK && trackInfo.prevState == ResourceState::READ ),
                        "Shouldn't be clearing a texture you just wrote to" );
                    TG_ASSERT( texInfo.state == ResourceState::WRITE, "Don't think reading from a just-cleared resource is intended" );
                    cTask->textureClears.emplace_back( index, texInfo.clearColor );

                    // image layout transition, if necessary
                    if ( trackInfo.currLayout != ImageLayout::TRANSFER_DST )
                    {
                        VkImageMemoryBarrier2 barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
                        barrier.srcStageMask     = VK_PIPELINE_STAGE_2_NONE;
                        barrier.srcAccessMask    = VK_ACCESS_2_NONE;
                        barrier.dstStageMask     = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
                        barrier.dstAccessMask    = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                        barrier.oldLayout        = PGToVulkanImageLayout( trackInfo.currLayout );
                        barrier.newLayout        = PGToVulkanImageLayout( ImageLayout::TRANSFER_DST );
                        barrier.image            = reinterpret_cast<VkImage>( index );
                        barrier.subresourceRange = ImageSubresourceRange( VK_IMAGE_ASPECT_COLOR_BIT ); // todo: support depth + stencil
                        cTask->imageBarriersPreClears.push_back( barrier );
                    }

                    // barrier to wait for the clear to be done before running the actual task
                    VkImageMemoryBarrier2 barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
                    barrier.srcStageMask     = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;
                    barrier.srcAccessMask    = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                    barrier.dstStageMask     = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
                    barrier.dstAccessMask    = VK_ACCESS_2_SHADER_WRITE_BIT;
                    barrier.oldLayout        = PGToVulkanImageLayout( ImageLayout::TRANSFER_DST );
                    barrier.newLayout        = PGToVulkanImageLayout( ImageLayout::GENERAL );
                    barrier.image            = reinterpret_cast<VkImage>( index );
                    barrier.subresourceRange = ImageSubresourceRange( VK_IMAGE_ASPECT_COLOR_BIT ); // todo: support depth + stencil
                    task->imageBarriers.push_back( barrier );
                }
                else if ( trackInfo.prevTask != NO_TASK && trackInfo.prevState != ResourceState::READ )
                {
                    // barrier to wait for any previous writes to be complete
                    VkImageMemoryBarrier2 barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
                    barrier.srcStageMask     = GetGeneralStageFlags( trackInfo.prevTaskType );
                    barrier.srcAccessMask    = GetAccessFlags( trackInfo.prevTaskType, trackInfo.prevState );
                    barrier.dstStageMask     = GetGeneralStageFlags( taskType );
                    barrier.dstAccessMask    = GetAccessFlags( taskType, texInfo.state );
                    barrier.oldLayout        = PGToVulkanImageLayout( ImageLayout::TRANSFER_DST );
                    barrier.newLayout        = PGToVulkanImageLayout( ImageLayout::GENERAL );
                    barrier.image            = reinterpret_cast<VkImage>( index );
                    barrier.subresourceRange = ImageSubresourceRange( VK_IMAGE_ASPECT_COLOR_BIT ); // todo: support depth + stencil
                    task->imageBarriers.push_back( barrier );
                }
                else if ( trackInfo.prevTask == NO_TASK )
                {
                    PG_ASSERT( trackInfo.currLayout == ImageLayout::UNDEFINED, "Should be the first use of this texture" );
                    VkImageMemoryBarrier2 barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
                    barrier.srcStageMask     = VK_PIPELINE_STAGE_2_NONE;
                    barrier.srcAccessMask    = VK_ACCESS_2_NONE;
                    barrier.dstStageMask     = GetGeneralStageFlags( taskType );
                    barrier.dstAccessMask    = GetAccessFlags( taskType, texInfo.state );
                    barrier.oldLayout        = PGToVulkanImageLayout( trackInfo.currLayout );
                    barrier.newLayout        = PGToVulkanImageLayout( ImageLayout::GENERAL );
                    barrier.image            = reinterpret_cast<VkImage>( index );
                    barrier.subresourceRange = ImageSubresourceRange( VK_IMAGE_ASPECT_COLOR_BIT ); // todo: support depth + stencil
                    task->imageBarriers.push_back( barrier );
                }

                if ( texInfo.state == ResourceState::READ || texInfo.state == ResourceState::READ_WRITE )
                    cTask->inputTextures.push_back( index );
                if ( texInfo.state == ResourceState::WRITE || texInfo.state == ResourceState::READ_WRITE )
                    cTask->outputTextures.push_back( index );

                trackInfo.currLayout   = ImageLayout::GENERAL;
                trackInfo.prevTask     = taskIndex;
                trackInfo.prevState    = texInfo.state;
                trackInfo.prevTaskType = taskType;
            }
        }
        else if ( taskType == TaskType::GRAPHICS )
        {
            GraphicsTaskBuilder* bgTask = static_cast<GraphicsTaskBuilder*>( builderTask );
            GraphicsTask* gTask         = new GraphicsTask;
            task                        = gTask;
            gTask->function             = bgTask->function;

            uint32_t minAttachWidth  = ~0u;
            uint32_t minAttachHeight = ~0u;
            for ( const TGBAttachmentInfo& bAttachInfo : bgTask->attachments )
            {
                TGResourceHandle texHandle      = bAttachInfo.ref.index;
                ResourceTrackingInfo& trackInfo = texTracking[texHandle];
                ImageLayout desiredLayout       = ImageLayout::COLOR_ATTACHMENT;
                if ( trackInfo.currLayout != desiredLayout )
                {
                    VkImageMemoryBarrier2 barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
                    barrier.srcStageMask     = GetGeneralStageFlags( trackInfo.prevTaskType );
                    barrier.srcAccessMask    = GetAccessFlags( trackInfo.prevTaskType, trackInfo.prevState );
                    barrier.dstStageMask     = GetGeneralStageFlags( taskType );
                    barrier.dstAccessMask    = GetAccessFlags( taskType, ResourceState::WRITE );
                    barrier.oldLayout        = PGToVulkanImageLayout( trackInfo.currLayout );
                    barrier.newLayout        = PGToVulkanImageLayout( desiredLayout );
                    barrier.image            = reinterpret_cast<VkImage>( texHandle );
                    barrier.subresourceRange = ImageSubresourceRange( VK_IMAGE_ASPECT_COLOR_BIT ); // todo: support depth + stencil
                    gTask->imageBarriers.push_back( barrier );
                }

                VkRenderingAttachmentInfo& vkAttach = gTask->attachments.emplace_back();
                vkAttach.sType                      = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
                vkAttach.pNext                      = nullptr;
                vkAttach.imageView                  = reinterpret_cast<VkImageView>( texHandle );
                vkAttach.imageLayout                = PGToVulkanImageLayout( desiredLayout );
                vkAttach.resolveMode                = VK_RESOLVE_MODE_NONE;
                vkAttach.resolveImageView           = VK_NULL_HANDLE;
                vkAttach.resolveImageLayout         = VK_IMAGE_LAYOUT_UNDEFINED;
                if ( bAttachInfo.isCleared )
                    vkAttach.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                else if ( trackInfo.currLayout == ImageLayout::UNDEFINED )
                    vkAttach.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                else
                    vkAttach.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                vkAttach.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                memcpy( vkAttach.clearValue.color.float32, &bAttachInfo.clearColor.x, sizeof( vec4 ) );

                minAttachWidth  = Min( minAttachWidth, textures[texHandle].GetWidth() );
                minAttachHeight = Min( minAttachHeight, textures[texHandle].GetHeight() );

                trackInfo.currLayout   = desiredLayout;
                trackInfo.prevTask     = taskIndex;
                trackInfo.prevState    = ResourceState::WRITE;
                trackInfo.prevTaskType = taskType;
            }

            gTask->renderingInfo.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
            gTask->renderingInfo.pNext                = nullptr;
            gTask->renderingInfo.flags                = 0;
            gTask->renderingInfo.renderArea           = { 0, 0, minAttachWidth, minAttachHeight };
            gTask->renderingInfo.layerCount           = 1;
            gTask->renderingInfo.viewMask             = 0;
            gTask->renderingInfo.colorAttachmentCount = (uint32_t)gTask->attachments.size();
            gTask->renderingInfo.pColorAttachments    = gTask->attachments.data();
            gTask->renderingInfo.pDepthAttachment     = nullptr;
            gTask->renderingInfo.pStencilAttachment   = nullptr;
        }
        else if ( taskType == TaskType::TRANSFER )
        {
            TransferTaskBuilder* btTask = static_cast<TransferTaskBuilder*>( builderTask );
            TransferTask* tTask         = new TransferTask;
            task                        = tTask;

            for ( const TGBTextureTransfer& transfer : btTask->textureBlits )
            {
                ResourceTrackingInfo& srcTrackInfo = texTracking[transfer.src.index];
                ResourceTrackingInfo& dstTrackInfo = texTracking[transfer.dst.index];
                PG_ASSERT( srcTrackInfo.prevTask != NO_TASK && srcTrackInfo.currLayout != ImageLayout::UNDEFINED,
                    "Need valid data to blit to another image!" );

                VkImageMemoryBarrier2 srcBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
                srcBarrier.srcStageMask     = GetGeneralStageFlags( srcTrackInfo.prevTaskType );
                srcBarrier.srcAccessMask    = GetAccessFlags( srcTrackInfo.prevTaskType, srcTrackInfo.prevState );
                srcBarrier.dstStageMask     = GetGeneralStageFlags( taskType );
                srcBarrier.dstAccessMask    = GetAccessFlags( taskType, ResourceState::READ );
                srcBarrier.oldLayout        = PGToVulkanImageLayout( srcTrackInfo.currLayout );
                srcBarrier.newLayout        = PGToVulkanImageLayout( ImageLayout::TRANSFER_SRC );
                srcBarrier.image            = reinterpret_cast<VkImage>( transfer.src.index );
                srcBarrier.subresourceRange = ImageSubresourceRange( VK_IMAGE_ASPECT_COLOR_BIT ); // todo: support depth + stencil
                tTask->imageBarriers.push_back( srcBarrier );

                VkImageMemoryBarrier2 dstBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
                dstBarrier.srcStageMask     = GetGeneralStageFlags( dstTrackInfo.prevTaskType );
                dstBarrier.srcAccessMask    = GetAccessFlags( dstTrackInfo.prevTaskType, dstTrackInfo.prevState );
                dstBarrier.dstStageMask     = GetGeneralStageFlags( taskType );
                dstBarrier.dstAccessMask    = GetAccessFlags( taskType, ResourceState::WRITE );
                dstBarrier.oldLayout        = PGToVulkanImageLayout( dstTrackInfo.currLayout );
                dstBarrier.newLayout        = PGToVulkanImageLayout( ImageLayout::TRANSFER_DST );
                dstBarrier.image            = reinterpret_cast<VkImage>( transfer.dst.index );
                dstBarrier.subresourceRange = ImageSubresourceRange( VK_IMAGE_ASPECT_COLOR_BIT ); // todo: support depth + stencil
                task->imageBarriers.push_back( dstBarrier );

                srcTrackInfo.currLayout   = ImageLayout::TRANSFER_SRC;
                srcTrackInfo.prevTask     = taskIndex;
                srcTrackInfo.prevState    = ResourceState::READ;
                srcTrackInfo.prevTaskType = taskType;

                dstTrackInfo.currLayout   = ImageLayout::TRANSFER_DST;
                dstTrackInfo.prevTask     = taskIndex;
                dstTrackInfo.prevState    = ResourceState::WRITE;
                dstTrackInfo.prevTaskType = taskType;

                tTask->textureBlits.emplace_back( transfer.dst.index, transfer.src.index );
            }
        }
        else if ( taskType == TaskType::PRESENT )
        {
            PresentTaskBuilder* bpTask = static_cast<PresentTaskBuilder*>( builderTask );
            PresentTask* pTask         = new PresentTask;
            task                       = pTask;

            ResourceTrackingInfo& trackInfo = texTracking[bpTask->presentationTex.index];
            PG_ASSERT( trackInfo.prevTask != NO_TASK && trackInfo.currLayout != ImageLayout::UNDEFINED,
                "Need to have written data to the image you're about to present!" );

            VkImageMemoryBarrier2 barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
            barrier.srcStageMask     = GetGeneralStageFlags( trackInfo.prevTaskType );
            barrier.srcAccessMask    = GetAccessFlags( trackInfo.prevTaskType, trackInfo.prevState );
            barrier.dstStageMask     = GetGeneralStageFlags( taskType );
            barrier.dstAccessMask    = GetAccessFlags( taskType, ResourceState::READ );
            barrier.oldLayout        = PGToVulkanImageLayout( trackInfo.currLayout );
            barrier.newLayout        = PGToVulkanImageLayout( ImageLayout::PRESENT_SRC_KHR );
            barrier.image            = reinterpret_cast<VkImage>( bpTask->presentationTex.index );
            barrier.subresourceRange = ImageSubresourceRange( VK_IMAGE_ASPECT_COLOR_BIT );
            pTask->imageBarriers.push_back( barrier );

            trackInfo.currLayout   = ImageLayout::PRESENT_SRC_KHR;
            trackInfo.prevTask     = taskIndex;
            trackInfo.prevState    = ResourceState::READ;
            trackInfo.prevTaskType = taskType;
        }
        else
        {
            PG_ASSERT( false, "Need to handle this new task type!" );
        }

        tasks.push_back( task );
    }
}

bool TaskGraph::Compile( TaskGraphBuilder& builder, CompileInfo& compileInfo )
{
    std::vector<ResourceData> resourceDatas;
    Compile_BuildResources( builder, compileInfo, resourceDatas );
    Compile_MemoryAliasing( builder, compileInfo, resourceDatas );
    Compile_SynchronizationAndTasks( builder, compileInfo );

    return true;
}

void TaskGraph::Free()
{
    for ( Task* task : tasks )
        delete task;

    // remove the external buffers and textures first
    for ( const auto& [resHandle, _] : externalBuffers )
    {
        buffers[resHandle].m_handle = VK_NULL_HANDLE;
    }
    for ( const auto& [resHandle, _] : externalTextures )
    {
        textures[resHandle].m_image = VK_NULL_HANDLE;
    }

    VmaAllocator allocator = rg.device.GetAllocator();
    for ( VmaAllocation& alloc : vmaAllocations )
    {
        vmaFreeMemory( allocator, alloc );
    }
    for ( Buffer& buf : buffers )
    {
        if ( buf.m_handle != VK_NULL_HANDLE )
            vmaDestroyBuffer( allocator, buf.m_handle, nullptr );
    }
    for ( Texture& tex : textures )
    {
        if ( tex.m_image != VK_NULL_HANDLE )
        {
            vmaDestroyImage( allocator, tex.m_image, nullptr );
            vkDestroyImageView( rg.device, tex.m_imageView, nullptr );
        }
    }

    tasks.clear();
    buffers.clear();
    textures.clear();
    vmaAllocations.clear();
    externalBuffers.clear();
    externalTextures.clear();
}

void TaskGraph::Execute( TGExecuteData& data )
{
    data.taskGraph = this;
    for ( auto& [bufHandle, extBufFunc] : externalBuffers )
    {
        buffers[bufHandle].m_handle = extBufFunc();
    }
    for ( auto& [texHandle, extTexFunc] : externalTextures )
    {
        Texture& tex = textures[texHandle];
        extTexFunc( tex.m_image, tex.m_imageView );
    }

    for ( Task* task : tasks )
    {
        task->Execute( &data );
    }
}

Buffer* TaskGraph::GetBuffer( TGResourceHandle handle ) { return &buffers[handle]; }
Texture* TaskGraph::GetTexture( TGResourceHandle handle ) { return &textures[handle]; }

} // namespace PG::Gfx
