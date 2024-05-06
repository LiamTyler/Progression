#include "r_taskGraph.hpp"
#include "internal/r_tg_resource_packing.hpp"
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
    builder->UpdateTextureLifetime( ref, taskHandle );
    textures.emplace_back( clearColor, ref, true, ResourceState::WRITE );
    return ref;
}
TGBTextureRef ComputeTaskBuilder::AddTextureOutput(
    const std::string& name, PixelFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint32_t mipLevels )
{
    TGBTextureRef ref = builder->AddTexture( name, format, width, height, depth, arrayLayers, mipLevels, nullptr );
    builder->UpdateTextureLifetime( ref, taskHandle );
    textures.emplace_back( vec4( 0 ), ref, false, ResourceState::WRITE );
    return ref;
}
void ComputeTaskBuilder::AddTextureOutput( TGBTextureRef& ref )
{
    builder->UpdateTextureLifetime( ref, taskHandle );
    for ( TGBTextureInfo& tInfo : textures )
    {
        if ( tInfo.ref == ref )
        {
            TG_ASSERT( tInfo.state == ResourceState::READ, "Don't output the same texture twice" );
            tInfo.state = ResourceState::READ_WRITE;
            return;
        }
    }
    textures.emplace_back( vec4( 0 ), ref, false, ResourceState::WRITE );
}
void ComputeTaskBuilder::AddTextureInput( TGBTextureRef& ref )
{
    builder->UpdateTextureLifetime( ref, taskHandle );
#if USING( TG_DEBUG )
    for ( TGBTextureInfo& tInfo : textures )
    {
        PG_ASSERT( tInfo.ref != ref, "If a resource is RW for this task, the call to AddTextureInput must come before the corresponding "
                                     "AddTextureOutput for this texture" );
    }
#endif // #if USING( TG_DEBUG )
    textures.emplace_back( vec4( 0 ), ref, false, ResourceState::READ );
}
TGBBufferRef ComputeTaskBuilder::AddBufferOutput(
    const std::string& name, BufferUsage bufferUsage, VmaMemoryUsage memoryUsage, size_t size, uint32_t clearVal )
{
    TGBBufferRef ref = builder->AddBuffer( name, bufferUsage, memoryUsage, size, nullptr );
    builder->UpdateBufferLifetime( ref, taskHandle );
    buffers.emplace_back( clearVal, ref, true, ResourceState::WRITE );
    return ref;
}
TGBBufferRef ComputeTaskBuilder::AddBufferOutput(
    const std::string& name, BufferUsage bufferUsage, VmaMemoryUsage memoryUsage, size_t size )
{
    TGBBufferRef ref = builder->AddBuffer( name, bufferUsage, memoryUsage, size, nullptr );
    builder->UpdateBufferLifetime( ref, taskHandle );
    buffers.emplace_back( 0, ref, false, ResourceState::WRITE );
    return ref;
}
void ComputeTaskBuilder::AddBufferOutput( TGBBufferRef& ref )
{
    builder->UpdateBufferLifetime( ref, taskHandle );
    for ( TGBBufferInfo& bInfo : buffers )
    {
        if ( bInfo.ref == ref )
        {
            TG_ASSERT( bInfo.state == ResourceState::READ, "Don't output the same buffer twice" );
            bInfo.state = ResourceState::READ_WRITE;
            return;
        }
    }
    buffers.emplace_back( 0, ref, false, ResourceState::WRITE );
}
void ComputeTaskBuilder::AddBufferInput( TGBBufferRef& ref )
{
    builder->UpdateBufferLifetime( ref, taskHandle );
#if USING( TG_DEBUG )
    for ( TGBBufferInfo& bInfo : buffers )
    {
        PG_ASSERT( bInfo.ref != ref, "If a resource is RW for this task, the call to AddBufferInput must come before the corresponding "
                                     "AddBufferOutput for this texture" );
    }
#endif // #if USING( TG_DEBUG )
    buffers.emplace_back( 0, ref, false, ResourceState::READ );
}
void ComputeTaskBuilder::SetFunction( ComputeFunction func ) { function = func; }

TaskGraphBuilder::TaskGraphBuilder() : numTasks( 0 )
{
    tasks.reserve( 128 );
    textures.reserve( 256 );
    buffers.reserve( 256 );
    textureLifetimes.reserve( 256 );
    bufferLifetimes.reserve( 256 );
}

TaskGraphBuilder::~TaskGraphBuilder()
{
    for ( TaskBuilder* task : tasks )
        delete task;
}

ComputeTaskBuilder* TaskGraphBuilder::AddComputeTask( const std::string& name )
{
    ComputeTaskBuilder* task = new ComputeTaskBuilder( this, numTasks++, name );
    tasks.push_back( task );
    return task;
}

TransferTaskBuilder* TaskGraphBuilder::AddTransferTask( const std::string& name )
{
    TransferTaskBuilder* task = new TransferTaskBuilder( this, numTasks++, name );
    tasks.push_back( task );
    return task;
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
    ResourceHandle index = static_cast<ResourceHandle>( textures.size() );

#if USING( TG_DEBUG )
    textures.emplace_back( name, width, height, depth, arrayLayers, mipLevels, format, func );
#else  // #if USING( TG_DEBUG )
    textures.emplace_back( width, height, depth, arrayLayers, mipLevels, format, func );
#endif // #else // #if USING( TG_DEBUG )
    textureLifetimes.emplace_back();

    TGBTextureRef ref;
    ref.index = index;

    return ref;
}

TGBBufferRef TaskGraphBuilder::AddBuffer(
    const std::string& name, BufferUsage bufferUsage, VmaMemoryUsage memoryUsage, size_t size, ExtBufferFunc func )
{
    ResourceHandle index = static_cast<ResourceHandle>( buffers.size() );
#if USING( TG_DEBUG )
    buffers.emplace_back( name, size, bufferUsage, memoryUsage, func );
#else  // #if USING( TG_DEBUG )
    buffers.emplace_back( length, type, format, func );
#endif // #else // #if USING( TG_DEBUG )
    bufferLifetimes.emplace_back();

    TGBBufferRef ref;
    ref.index = index;

    return ref;
}

void TaskGraphBuilder::UpdateTextureLifetime( TGBTextureRef ref, TaskHandle task )
{
    ResLifetime& lifetime = textureLifetimes[ref.index];
    lifetime.firstTask    = Min( lifetime.firstTask, task.index );
    lifetime.lastTask     = Max( lifetime.lastTask, task.index );
}

void TaskGraphBuilder::UpdateBufferLifetime( TGBBufferRef ref, TaskHandle task )
{
    ResLifetime& lifetime = bufferLifetimes[ref.index];
    lifetime.firstTask    = Min( lifetime.firstTask, task.index );
    lifetime.lastTask     = Max( lifetime.lastTask, task.index );
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

static VkAccessFlags2 GetSrcAccessFlags( TaskType taskType, ResourceState resState )
{
    switch ( taskType )
    {
    case TaskType::NONE: return VK_ACCESS_2_NONE;
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
        PG_ASSERT( false, "todo" );
    }
    case TaskType::TRANSFER:
    {
        TG_ASSERT( resState != ResourceState::READ_WRITE );
        return resState == ResourceState::READ ? VK_ACCESS_2_TRANSFER_READ_BIT : VK_ACCESS_2_TRANSFER_WRITE_BIT;
    }
    }

    return VK_ACCESS_2_NONE;
}

static VkAccessFlags2 GetDstAccessFlags( TaskType taskType, ResourceState resState )
{
    TG_ASSERT( taskType != TaskType::NONE );
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
        PG_ASSERT( false, "todo" );
    }
    case TaskType::TRANSFER:
    {
        TG_ASSERT( resState != ResourceState::READ_WRITE );
        return resState == ResourceState::READ ? VK_ACCESS_2_TRANSFER_READ_BIT : VK_ACCESS_2_TRANSFER_WRITE_BIT;
    }
    }

    return VK_ACCESS_2_NONE;
}

bool TaskGraph::Compile( TaskGraphBuilder& builder, TaskGraphCompileInfo& compileInfo )
{
    std::vector<ResourceData> resourceDatas;
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

        gfxTex.m_desc               = desc;
        gfxTex.m_image              = VK_NULL_HANDLE;
        gfxTex.m_imageView          = VK_NULL_HANDLE;
        gfxTex.m_device             = rg.device.GetHandle();
        gfxTex.m_sampler            = GetSampler( desc.sampler );
        gfxTex.m_bindlessArrayIndex = PG_INVALID_TEXTURE_INDEX;

        if ( buildTex.externalFunc )
        {
            externalTextures.emplace_back( (ResourceHandle)i, buildTex.externalFunc );
            continue;
        }

        ResourceData& data = resourceDatas.emplace_back();
        data.resType       = ResourceType::TEXTURE;
        data.texRef        = { (ResourceHandle)i };
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
            externalBuffers.emplace_back( (ResourceHandle)i, buildBuffer.externalFunc );
            continue;
        }

        ResourceData& data = resourceDatas.emplace_back();
        data.resType       = ResourceType::BUFFER;
        data.bufRef        = { (ResourceHandle)i };
        data.firstTask     = builder.textureLifetimes[i].firstTask;
        data.lastTask      = builder.textureLifetimes[i].lastTask;

        VkBufferCreateInfo info{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        info.usage = PGToVulkanBufferType( gfxBuffer.m_bufferUsage ) | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        info.size  = buildBuffer.size;
        VK_CHECK( vkCreateBuffer( rg.device, &info, nullptr, &data.buffer ) );
        gfxBuffer.m_handle = data.buffer;
        vkGetBufferMemoryRequirements( rg.device, data.buffer, &data.memoryReq );

#if USING( TG_DEBUG )
        PG_DEBUG_MARKER_SET_BUFFER_NAME( gfxBuffer, buildBuffer.debugName );
#endif // #if USING( TG_DEBUG )
    }

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
#endif // #if USING( TG_DEBUG )
    }

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
                ResourceHandle index            = bufInfo.ref.index;
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
                    barrier.dstAccessMask = GetDstAccessFlags( taskType, bufInfo.state );
                    barrier.size          = VK_WHOLE_SIZE;
                    barrier.buffer        = reinterpret_cast<VkBuffer>( index );
                    task->bufferBarriers.push_back( barrier );
                }
                else if ( trackInfo.prevTask != NO_TASK && trackInfo.prevState != ResourceState::READ )
                {
                    // barrier to wait for any previous writes to be complete
                    VkBufferMemoryBarrier2 barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2 };
                    barrier.srcStageMask  = GetGeneralStageFlags( trackInfo.prevTaskType );
                    barrier.srcAccessMask = GetSrcAccessFlags( trackInfo.prevTaskType, trackInfo.prevState );
                    barrier.dstStageMask  = GetDstStageFlags( taskType, buildBuffer.bufferUsage );
                    barrier.dstAccessMask = GetDstAccessFlags( taskType, bufInfo.state );
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
                ResourceHandle index = texInfo.ref.index;
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
                    barrier.srcAccessMask    = GetSrcAccessFlags( trackInfo.prevTaskType, trackInfo.prevState );
                    barrier.dstStageMask     = GetGeneralStageFlags( taskType );
                    barrier.dstAccessMask    = GetDstAccessFlags( taskType, texInfo.state );
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
                    barrier.dstAccessMask    = GetDstAccessFlags( taskType, texInfo.state );
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
            PG_ASSERT( false, "todo" );
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
                srcBarrier.srcAccessMask    = GetSrcAccessFlags( srcTrackInfo.prevTaskType, srcTrackInfo.prevState );
                srcBarrier.dstStageMask     = GetGeneralStageFlags( taskType );
                srcBarrier.dstAccessMask    = GetDstAccessFlags( taskType, ResourceState::READ );
                srcBarrier.oldLayout        = PGToVulkanImageLayout( srcTrackInfo.currLayout );
                srcBarrier.newLayout        = PGToVulkanImageLayout( ImageLayout::TRANSFER_SRC );
                srcBarrier.image            = reinterpret_cast<VkImage>( transfer.src.index );
                srcBarrier.subresourceRange = ImageSubresourceRange( VK_IMAGE_ASPECT_COLOR_BIT ); // todo: support depth + stencil
                tTask->imageBarriers.push_back( srcBarrier );

                VkImageMemoryBarrier2 dstBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
                dstBarrier.srcStageMask     = GetGeneralStageFlags( dstTrackInfo.prevTaskType );
                dstBarrier.srcAccessMask    = GetSrcAccessFlags( dstTrackInfo.prevTaskType, dstTrackInfo.prevState );
                dstBarrier.dstStageMask     = GetGeneralStageFlags( taskType );
                dstBarrier.dstAccessMask    = GetDstAccessFlags( taskType, ResourceState::WRITE );
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

        tasks.push_back( task );
    }

    return true;
}

static void FillBufferBarriers(
    TaskGraph* taskGraph, std::vector<VkBufferMemoryBarrier2>& scratch, std::vector<VkBufferMemoryBarrier2>& source )
{
    scratch = source;
    for ( VkBufferMemoryBarrier2& barrier : scratch )
    {
        ResourceHandle bufHandle;
        memcpy( &bufHandle, &barrier.buffer, sizeof( ResourceHandle ) );
        barrier.buffer = taskGraph->GetBuffer( bufHandle )->GetHandle();
    }
}

static void FillImageBarriers(
    TaskGraph* taskGraph, std::vector<VkImageMemoryBarrier2>& scratch, std::vector<VkImageMemoryBarrier2>& source )
{
    scratch = source;
    for ( VkImageMemoryBarrier2& barrier : scratch )
    {
        ResourceHandle imgHandle;
        memcpy( &imgHandle, &barrier.image, sizeof( ResourceHandle ) );
        barrier.image = taskGraph->GetTexture( imgHandle )->GetImage();
    }
}

void Task::SubmitBarriers( TGExecuteData& data )
{
    if ( imageBarriers.empty() && bufferBarriers.empty() )
        return;

    FillImageBarriers( data.taskGraph, data.scratchImageBarriers, imageBarriers );
    FillBufferBarriers( data.taskGraph, data.scratchBufferBarriers, bufferBarriers );

    VkDependencyInfo depInfo{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
    depInfo.imageMemoryBarrierCount  = (uint32_t)data.scratchImageBarriers.size();
    depInfo.pImageMemoryBarriers     = data.scratchImageBarriers.data();
    depInfo.bufferMemoryBarrierCount = (uint32_t)data.scratchBufferBarriers.size();
    depInfo.pBufferMemoryBarriers    = data.scratchBufferBarriers.data();

    vkCmdPipelineBarrier2( *data.cmdBuf, &depInfo );
}

void ComputeTask::Execute( TGExecuteData& data )
{
    if ( !imageBarriersPreClears.empty() )
    {
        FillImageBarriers( data.taskGraph, data.scratchImageBarriers, imageBarriersPreClears );
        VkDependencyInfo depInfo{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
        depInfo.imageMemoryBarrierCount = (uint32_t)data.scratchImageBarriers.size();
        depInfo.pImageMemoryBarriers    = data.scratchImageBarriers.data();

        vkCmdPipelineBarrier2( *data.cmdBuf, &depInfo );
    }

    for ( const BufferClearSubTask& clear : bufferClears )
    {
        Buffer* buf = data.taskGraph->GetBuffer( clear.bufferHandle );
        vkCmdFillBuffer( *data.cmdBuf, buf->GetHandle(), 0, VK_WHOLE_SIZE, clear.clearVal );
    }
    for ( const TextureClearSubTask& clear : textureClears )
    {
        Texture* tex = data.taskGraph->GetTexture( clear.textureHandle );
        VkClearColorValue clearColor;
        memcpy( &clearColor, &clear.clearVal.x, sizeof( vec4 ) );
        VkImageSubresourceRange range = ImageSubresourceRange( VK_IMAGE_ASPECT_COLOR_BIT );
        vkCmdClearColorImage( *data.cmdBuf, tex->GetImage(), PGToVulkanImageLayout( ImageLayout::TRANSFER_DST ), &clearColor, 1, &range );
    }

    SubmitBarriers( data );

    function( this, &data );
}

void TransferTask::Execute( TGExecuteData& data )
{
    SubmitBarriers( data );

    for ( const TextureTransfer& texBlit : textureBlits )
    {
        Texture* srcTex = data.taskGraph->GetTexture( texBlit.src );
        Texture* dstTex = data.taskGraph->GetTexture( texBlit.dst );

        VkImageBlit2 blitRegion{ VK_STRUCTURE_TYPE_IMAGE_BLIT_2 };
        blitRegion.srcOffsets[1].x = srcTex->GetWidth();
        blitRegion.srcOffsets[1].y = srcTex->GetHeight();
        blitRegion.srcOffsets[1].z = 1;

        blitRegion.dstOffsets[1].x = dstTex->GetWidth();
        blitRegion.dstOffsets[1].y = dstTex->GetHeight();
        blitRegion.dstOffsets[1].z = 1;

        blitRegion.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        blitRegion.srcSubresource.baseArrayLayer = 0;
        blitRegion.srcSubresource.layerCount     = 1;
        blitRegion.srcSubresource.mipLevel       = 0;

        blitRegion.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        blitRegion.dstSubresource.baseArrayLayer = 0;
        blitRegion.dstSubresource.layerCount     = 1;
        blitRegion.dstSubresource.mipLevel       = 0;

        VkBlitImageInfo2 blitInfo{ VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2 };
        blitInfo.dstImage       = dstTex->GetImage();
        blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        blitInfo.srcImage       = srcTex->GetImage();
        blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        blitInfo.filter         = VK_FILTER_LINEAR;
        blitInfo.regionCount    = 1;
        blitInfo.pRegions       = &blitRegion;

        vkCmdBlitImage2( *data.cmdBuf, &blitInfo );
    }
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
        task->Execute( data );
    }
}

Buffer* TaskGraph::GetBuffer( ResourceHandle handle ) { return &buffers[handle]; }
Texture* TaskGraph::GetTexture( ResourceHandle handle ) { return &textures[handle]; }

} // namespace PG::Gfx
