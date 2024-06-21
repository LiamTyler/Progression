#include "r_tg_builder.hpp"
#include "renderer/graphics_api/pg_to_vulkan_types.hpp"
#include "renderer/r_globals.hpp"

namespace PG::Gfx
{

TGBTextureRef PipelineTaskBuilder::AddTextureOutput(
    std::string_view name, PixelFormat format, const vec4& clearColor, u32 width, u32 height, u32 depth, u32 arrayLayers, u32 mipLevels )
{
    TGBTextureRef ref = builder->AddTexture( name, format, width, height, depth, arrayLayers, mipLevels, nullptr,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, taskHandle.index );
    textures.emplace_back( clearColor, ref, true, ResourceState::WRITE );
    return ref;
}

TGBTextureRef PipelineTaskBuilder::AddTextureOutput(
    std::string_view name, PixelFormat format, u32 width, u32 height, u32 depth, u32 arrayLayers, u32 mipLevels )
{
    TGBTextureRef ref = builder->AddTexture(
        name, format, width, height, depth, arrayLayers, mipLevels, nullptr, VK_IMAGE_USAGE_STORAGE_BIT, taskHandle.index );
    textures.emplace_back( vec4( 0 ), ref, false, ResourceState::WRITE );
    return ref;
}

void PipelineTaskBuilder::AddTextureOutput( TGBTextureRef& ref )
{
    builder->UpdateTextureLifetimeAndUsage( ref, taskHandle, VK_IMAGE_USAGE_STORAGE_BIT );
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

void PipelineTaskBuilder::AddTextureInput( TGBTextureRef& ref )
{
    builder->UpdateTextureLifetimeAndUsage( ref, taskHandle, VK_IMAGE_USAGE_STORAGE_BIT );
#if USING( TG_DEBUG )
    for ( TGBTextureInfo& tInfo : textures )
    {
        PG_ASSERT( tInfo.ref != ref, "If a resource is RW for this task, the call to AddTextureInput must come before the corresponding "
                                     "AddTextureOutput for this texture" );
    }
#endif // #if USING( TG_DEBUG )
    textures.emplace_back( vec4( 0 ), ref, false, ResourceState::READ );
}

TGBBufferRef PipelineTaskBuilder::AddBufferOutput( std::string_view name, VmaMemoryUsage memoryUsage, size_t size, u32 clearVal )
{
    TGBBufferRef ref = builder->AddBuffer( name, BufferUsage::STORAGE, memoryUsage, size, nullptr, taskHandle.index );
    buffers.emplace_back( clearVal, ref, true, ResourceState::WRITE, BufferUsage::STORAGE );
    return ref;
}

TGBBufferRef PipelineTaskBuilder::AddBufferOutput( std::string_view name, VmaMemoryUsage memoryUsage, size_t size )
{
    TGBBufferRef ref = builder->AddBuffer( name, BufferUsage::STORAGE, memoryUsage, size, nullptr, taskHandle.index );
    buffers.emplace_back( 0, ref, false, ResourceState::WRITE, BufferUsage::STORAGE );
    return ref;
}

void PipelineTaskBuilder::AddBufferOutput( TGBBufferRef& ref )
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

void PipelineTaskBuilder::AddBufferInput( TGBBufferRef& ref, BufferUsage usageForThisTask )
{
    builder->UpdateBufferLifetime( ref, taskHandle );
    builder->buffers[ref.index].bufferUsage |= usageForThisTask;
#if USING( TG_DEBUG )
    for ( TGBBufferInfo& bInfo : buffers )
    {
        PG_ASSERT( bInfo.ref != ref, "If a resource is RW for this task, the call to AddBufferInput must come before the corresponding "
                                     "AddBufferOutput for this texture" );
    }
#endif // #if USING( TG_DEBUG )
    buffers.emplace_back( 0, ref, false, ResourceState::READ, usageForThisTask );
}

void ComputeTaskBuilder::SetFunction( ComputeFunction func ) { function = func; }

TGBTextureRef GraphicsTaskBuilder::AddColorAttachment(
    std::string_view name, PixelFormat format, const vec4& clearColor, u32 width, u32 height, u32 depth, u32 arrayLayers, u32 mipLevels )
{
    TGBTextureRef ref = builder->AddTexture(
        name, format, width, height, depth, arrayLayers, mipLevels, nullptr, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, taskHandle.index );
    attachments.emplace_back( clearColor, ref, ResourceType::COLOR_ATTACH, true );
    return ref;
}

TGBTextureRef GraphicsTaskBuilder::AddColorAttachment(
    std::string_view name, PixelFormat format, u32 width, u32 height, u32 depth, u32 arrayLayers, u32 mipLevels )
{
    TGBTextureRef ref = builder->AddTexture(
        name, format, width, height, depth, arrayLayers, mipLevels, nullptr, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, taskHandle.index );
    attachments.emplace_back( vec4( 0 ), ref, ResourceType::COLOR_ATTACH, false );
    return ref;
}

void GraphicsTaskBuilder::AddColorAttachment( TGBTextureRef tex )
{
    builder->UpdateTextureLifetimeAndUsage( tex, taskHandle, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT );
    attachments.emplace_back( vec4( 0 ), tex, ResourceType::COLOR_ATTACH, false );
}

TGBTextureRef GraphicsTaskBuilder::AddDepthAttachment( std::string_view name, PixelFormat format, u32 width, u32 height, f32 clearVal )
{
    TGBTextureRef ref =
        builder->AddTexture( name, format, width, height, 1, 1, 1, nullptr, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, taskHandle.index );
    attachments.emplace_back( vec4( clearVal ), ref, ResourceType::DEPTH_ATTACH, true );
    return ref;
}

TGBTextureRef GraphicsTaskBuilder::AddDepthAttachment( std::string_view name, PixelFormat format, u32 width, u32 height )
{
    TGBTextureRef ref =
        builder->AddTexture( name, format, width, height, 1, 1, 1, nullptr, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, taskHandle.index );
    attachments.emplace_back( vec4( 0 ), ref, ResourceType::DEPTH_ATTACH, false );
    return ref;
}

void GraphicsTaskBuilder::AddDepthAttachment( TGBTextureRef tex )
{
    builder->UpdateTextureLifetimeAndUsage( tex, taskHandle, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT );
    attachments.emplace_back( vec4( 0 ), tex, ResourceType::DEPTH_ATTACH, false );
}

void GraphicsTaskBuilder::SetFunction( GraphicsFunction func ) { function = func; }

void TransferTaskBuilder::BlitTexture( TGBTextureRef dst, TGBTextureRef src )
{
    textureBlits.emplace_back( dst, src );
    builder->UpdateTextureLifetimeAndUsage( src, taskHandle, VK_IMAGE_USAGE_TRANSFER_SRC_BIT );
    builder->UpdateTextureLifetimeAndUsage( dst, taskHandle, VK_IMAGE_USAGE_TRANSFER_DST_BIT );
}

void PresentTaskBuilder::SetPresentationImage( TGBTextureRef tex )
{
    presentationTex = tex;
    builder->UpdateTextureLifetimeAndUsage( tex, taskHandle );
}

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

ComputeTaskBuilder* TaskGraphBuilder::AddComputeTask( std::string_view name )
{
    ComputeTaskBuilder* task = new ComputeTaskBuilder( this, numTasks++, name );
    tasks.push_back( task );
    return task;
}

GraphicsTaskBuilder* TaskGraphBuilder::AddGraphicsTask( std::string_view name )
{
    GraphicsTaskBuilder* task = new GraphicsTaskBuilder( this, numTasks++, name );
    tasks.push_back( task );
    return task;
}

TransferTaskBuilder* TaskGraphBuilder::AddTransferTask( std::string_view name )
{
    TransferTaskBuilder* task = new TransferTaskBuilder( this, numTasks++, name );
    tasks.push_back( task );
    return task;
}

PresentTaskBuilder* TaskGraphBuilder::AddPresentTask()
{
    PresentTaskBuilder* task = new PresentTaskBuilder( this, numTasks++, "Final Presentation" );
    tasks.push_back( task );
    return task;
}

TGBTextureRef TaskGraphBuilder::RegisterExternalTexture(
    std::string_view name, PixelFormat format, u32 width, u32 height, u32 depth, u32 arrayLayers, u32 mipLevels, ExtTextureFunc func )
{
    return AddTexture( name, format, width, height, depth, arrayLayers, mipLevels, func, 0, UINT16_MAX );
}

TGBBufferRef TaskGraphBuilder::RegisterExternalBuffer(
    std::string_view name, BufferUsage bufferUsage, VmaMemoryUsage memoryUsage, size_t size, ExtBufferFunc func )
{
    return AddBuffer( name, bufferUsage, memoryUsage, size, func, UINT16_MAX );
}

TGBTextureRef TaskGraphBuilder::AddTexture( std::string_view name, PixelFormat format, u32 width, u32 height, u32 depth, u32 arrayLayers,
    u32 mipLevels, ExtTextureFunc func, VkImageUsageFlags usage, u16 taskIndex )
{
    TGResourceHandle index = static_cast<TGResourceHandle>( textures.size() );

#if USING( TG_DEBUG )
    textures.emplace_back( std::string( name ), width, height, depth, arrayLayers, mipLevels, format, usage, func );
#else  // #if USING( TG_DEBUG )
    textures.emplace_back( width, height, depth, arrayLayers, mipLevels, format, usage, func );
#endif // #else // #if USING( TG_DEBUG )
    if ( taskIndex == UINT16_MAX )
        textureLifetimes.emplace_back();
    else
        textureLifetimes.emplace_back( taskIndex, taskIndex );

    TGBTextureRef ref;
    ref.index = index;

    return ref;
}

TGBBufferRef TaskGraphBuilder::AddBuffer(
    std::string_view name, BufferUsage bufferUsage, VmaMemoryUsage memoryUsage, size_t size, ExtBufferFunc func, u16 taskIndex )
{
    TGResourceHandle index = static_cast<TGResourceHandle>( buffers.size() );
#if USING( TG_DEBUG )
    buffers.emplace_back( std::string( name ), size, bufferUsage, memoryUsage, func );
#else  // #if USING( TG_DEBUG )
    buffers.emplace_back( size, bufferUsage, memoryUsage, func );
#endif // #else // #if USING( TG_DEBUG )
    if ( taskIndex == UINT16_MAX )
        bufferLifetimes.emplace_back();
    else
        bufferLifetimes.emplace_back( taskIndex, taskIndex );

    TGBBufferRef ref;
    ref.index = index;

    return ref;
}

void TaskGraphBuilder::UpdateTextureLifetimeAndUsage( TGBTextureRef ref, TaskHandle task, VkImageUsageFlags flags )
{
    ResLifetime& lifetime = textureLifetimes[ref.index];
    lifetime.firstTask    = Min( lifetime.firstTask, task.index );
    lifetime.lastTask     = Max( lifetime.lastTask, task.index );
    textures[ref.index].usage |= flags;
}

void TaskGraphBuilder::UpdateBufferLifetime( TGBBufferRef ref, TaskHandle task )
{
    ResLifetime& lifetime = bufferLifetimes[ref.index];
    lifetime.firstTask    = Min( lifetime.firstTask, task.index );
    lifetime.lastTask     = Max( lifetime.lastTask, task.index );
}

} // namespace PG::Gfx
