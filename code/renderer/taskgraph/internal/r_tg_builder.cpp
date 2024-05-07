#include "r_tg_builder.hpp"
#include "renderer/graphics_api/pg_to_vulkan_types.hpp"
#include "renderer/r_globals.hpp"

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

void TransferTaskBuilder::BlitTexture( TGBTextureRef dst, TGBTextureRef src ) { textureBlits.emplace_back( dst, src ); }

void PresentTaskBuilder::SetPresentationImage( TGBTextureRef tex ) { presentationTex = tex; }

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

PresentTaskBuilder* TaskGraphBuilder::AddPresentTask()
{
    PresentTaskBuilder* task = new PresentTaskBuilder( this, numTasks++, "Final Presentation" );
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
    TGResourceHandle index = static_cast<TGResourceHandle>( textures.size() );

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
    TGResourceHandle index = static_cast<TGResourceHandle>( buffers.size() );
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

} // namespace PG::Gfx
