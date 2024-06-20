#pragma once

#include "r_tg_tasks.hpp"
#include "renderer/graphics_api/buffer.hpp"
#include "renderer/graphics_api/texture.hpp"
#include <functional>

namespace PG::Gfx
{

enum class RelativeSizes : u32
{
    Scene   = 1u << 30,
    Display = 1u << 31,

    ALL = Scene | Display
};

constexpr u32 AUTO_FULL_MIP_CHAIN() { return UINT32_MAX; }
constexpr u32 SIZE_SCENE() { return static_cast<u32>( RelativeSizes::Scene ); }
constexpr u32 SIZE_DISPLAY() { return static_cast<u32>( RelativeSizes::Display ); }
constexpr u32 SIZE_SCENE_DIV( u32 x ) { return static_cast<u32>( RelativeSizes::Scene ) | x; }
constexpr u32 SIZE_DISPLAY_DIV( u32 x ) { return static_cast<u32>( RelativeSizes::Display ) | x; }
constexpr u32 ResolveRelativeSize( u32 scene, u32 display, u32 relSize )
{
    u32 size = relSize & ~(u32)RelativeSizes::ALL;
    if ( relSize & (u32)RelativeSizes::Scene )
    {
        return size == 0 ? scene : scene / size;
    }
    else if ( relSize & (u32)RelativeSizes::Display )
    {
        return size == 0 ? display : display / size;
    }
    else
    {
        return relSize;
    }
}

using ExtTextureFunc = std::function<Texture()>;
using ExtBufferFunc  = std::function<Buffer()>;

struct TGBTexture
{
    TG_DEBUG_ONLY( std::string debugName );
    u32 width;
    u32 height;
    u32 depth;
    u8 arrayLayers;
    u8 mipLevels;
    PixelFormat format;
    VkImageUsageFlags usage;
    ExtTextureFunc externalFunc;
};

struct TGBBuffer
{
    TG_DEBUG_ONLY( std::string debugName );
    size_t size;
    BufferUsage bufferUsage;
    VmaMemoryUsage memoryUsage;
    ExtBufferFunc externalFunc;
};

enum class TaskType : u16
{
    NONE     = 0, // only valid internally for signaling a resource has no previous task yet (first usage)
    COMPUTE  = 1,
    GRAPHICS = 2,
    TRANSFER = 3,
    PRESENT  = 4,
};

struct TaskHandle
{
    u16 index : 13;
    TaskType type : 3;

    TaskHandle( u16 inIndex, TaskType inType ) : index( inIndex ), type( inType ) {}
};

struct TGBTextureRef
{
    TGResourceHandle index;

    bool operator==( const TGBTextureRef& o ) const { return index == o.index; }
};

struct TGBBufferRef
{
    TGResourceHandle index;

    bool operator==( const TGBBufferRef& o ) const { return index == o.index; }
};

struct TGBBufferInfo
{
    u32 clearVal;
    TGBBufferRef ref;
    bool isCleared;
    ResourceState state;
};

struct TGBTextureInfo
{
    vec4 clearColor;
    TGBTextureRef ref;
    bool isCleared;
    ResourceState state;
};

class TaskGraphBuilder;

class TaskBuilder
{
public:
    TaskBuilder( TaskGraphBuilder* inBuilder, TaskHandle inTaskHandle, std::string_view inName )
        : builder( inBuilder ), taskHandle( inTaskHandle )
#if USING( PG_GPU_PROFILING ) || USING( TG_DEBUG )
          ,
          name( inName )
#endif // #if USING( PG_GPU_PROFILING ) || USING( TG_DEBUG )
    {
    }

    virtual ~TaskBuilder() = default;

    TaskGraphBuilder* const builder;
    const TaskHandle taskHandle;
#if USING( PG_GPU_PROFILING ) || USING( TG_DEBUG )
    std::string name;
#endif // #if USING( PG_GPU_PROFILING ) || USING( TG_DEBUG )
};

class PipelineTaskBuilder : public TaskBuilder
{
public:
    PipelineTaskBuilder( TaskGraphBuilder* inBuilder, u16 taskIndex, TaskType taskType, std::string_view inName )
        : TaskBuilder( inBuilder, TaskHandle( taskIndex, taskType ), inName )
    {
    }

    TGBTextureRef AddTextureOutput( std::string_view name, PixelFormat format, const vec4& clearColor, u32 width, u32 height, u32 depth = 1,
        u32 arrayLayers = 1, u32 mipLevels = 1 );
    TGBTextureRef AddTextureOutput(
        std::string_view name, PixelFormat format, u32 width, u32 height, u32 depth = 1, u32 arrayLayers = 1, u32 mipLevels = 1 );
    void AddTextureOutput( TGBTextureRef& texture );
    void AddTextureInput( TGBTextureRef& texture );

    TGBBufferRef AddBufferOutput( std::string_view name, BufferUsage bufferUsage, VmaMemoryUsage memoryUsage, size_t size, u32 clearVal );
    TGBBufferRef AddBufferOutput( std::string_view name, BufferUsage bufferUsage, VmaMemoryUsage memoryUsage, size_t size );
    void AddBufferOutput( TGBBufferRef& buffer );
    void AddBufferInput( TGBBufferRef& buffer );

    std::vector<TGBBufferInfo> buffers;
    std::vector<TGBTextureInfo> textures;
};

class ComputeTaskBuilder : public PipelineTaskBuilder
{
public:
    ComputeTaskBuilder( TaskGraphBuilder* inBuilder, u16 taskIndex, std::string_view inName )
        : PipelineTaskBuilder( inBuilder, taskIndex, TaskType::COMPUTE, inName )
    {
    }

    void SetFunction( ComputeFunction func );

    ComputeFunction function;
};

struct TGBAttachmentInfo
{
    vec4 clearColor;
    TGBTextureRef ref;
    ResourceType type;
    bool isCleared;
};

class GraphicsTaskBuilder : public PipelineTaskBuilder
{
public:
    GraphicsTaskBuilder( TaskGraphBuilder* inBuilder, u16 taskIndex, std::string_view inName )
        : PipelineTaskBuilder( inBuilder, taskIndex, TaskType::GRAPHICS, inName )
    {
    }

    TGBTextureRef AddColorAttachment( std::string_view name, PixelFormat format, const vec4& clearColor, u32 width, u32 height,
        u32 depth = 1, u32 arrayLayers = 1, u32 mipLevels = 1 );
    TGBTextureRef AddColorAttachment(
        std::string_view name, PixelFormat format, u32 width, u32 height, u32 depth = 1, u32 arrayLayers = 1, u32 mipLevels = 1 );
    void AddColorAttachment( TGBTextureRef tex );

    TGBTextureRef AddDepthAttachment( std::string_view name, PixelFormat format, u32 width, u32 height, f32 clearVal );
    TGBTextureRef AddDepthAttachment( std::string_view name, PixelFormat format, u32 width, u32 height );
    void AddDepthAttachment( TGBTextureRef tex );

    void SetFunction( GraphicsFunction func );

    std::vector<TGBAttachmentInfo> attachments;
    GraphicsFunction function;
};

struct TGBTextureTransfer
{
    TGBTextureRef dst;
    TGBTextureRef src;
};

class TransferTaskBuilder : public TaskBuilder
{
public:
    TransferTaskBuilder( TaskGraphBuilder* inBuilder, u16 taskIndex, std::string_view inName )
        : TaskBuilder( inBuilder, TaskHandle( taskIndex, TaskType::TRANSFER ), inName )
    {
    }

    void BlitTexture( TGBTextureRef dst, TGBTextureRef src );

    std::vector<TGBTextureTransfer> textureBlits;
};

class PresentTaskBuilder : public TaskBuilder
{
public:
    PresentTaskBuilder( TaskGraphBuilder* inBuilder, u16 taskIndex, std::string_view inName )
        : TaskBuilder( inBuilder, TaskHandle( taskIndex, TaskType::PRESENT ), inName )
    {
    }

    void SetPresentationImage( TGBTextureRef tex );

    TGBTextureRef presentationTex;
};

static constexpr u16 NO_TASK = UINT16_MAX;
struct ResourceTrackingInfo
{
    ImageLayout currLayout = ImageLayout::UNDEFINED;
    u16 prevTask           = NO_TASK;
    TaskType prevTaskType  = TaskType::NONE;
    ResourceState prevState;
    ResourceType prevResType = ResourceType::NONE;

    ResourceTrackingInfo() = default;
    ResourceTrackingInfo( u16 task, TaskType type, ResourceState state, ResourceType rType )
        : prevTask( task ), prevTaskType( type ), prevState( state ), prevResType( rType )
    {
    }
    ResourceTrackingInfo( ImageLayout layout, u16 task, TaskType type, ResourceState state, ResourceType rType )
        : currLayout( layout ), prevTask( task ), prevTaskType( type ), prevState( state ), prevResType( rType )
    {
    }
};

class TaskGraphBuilder
{
    friend class PipelineTaskBuilder;
    friend class ComputeTaskBuilder;
    friend class GraphicsTaskBuilder;
    friend class TransferTaskBuilder;
    friend class PresentTaskBuilder;
    friend class TaskGraph;

public:
    TaskGraphBuilder();
    ~TaskGraphBuilder();
    ComputeTaskBuilder* AddComputeTask( std::string_view name );
    GraphicsTaskBuilder* AddGraphicsTask( std::string_view name );
    TransferTaskBuilder* AddTransferTask( std::string_view name );
    PresentTaskBuilder* AddPresentTask();

    TGBTextureRef RegisterExternalTexture(
        std::string_view name, PixelFormat format, u32 width, u32 height, u32 depth, u32 arrayLayers, u32 mipLevels, ExtTextureFunc func );
    TGBBufferRef RegisterExternalBuffer(
        std::string_view name, BufferUsage bufferUsage, VmaMemoryUsage memoryUsage, size_t size, ExtBufferFunc func );

private:
    TGBTextureRef AddTexture( std::string_view name, PixelFormat format, u32 width, u32 height, u32 depth, u32 arrayLayers, u32 mipLevels,
        ExtTextureFunc func, VkImageUsageFlags usage, u16 taskIndex );
    TGBBufferRef AddBuffer(
        std::string_view name, BufferUsage bufferUsage, VmaMemoryUsage memoryUsage, size_t size, ExtBufferFunc func, u16 taskIndex );

    std::vector<TaskBuilder*> tasks;
    std::vector<TGBTexture> textures;
    std::vector<TGBBuffer> buffers;

    struct ResLifetime
    {
        u16 firstTask = UINT16_MAX;
        u16 lastTask  = 0;
    };
    std::vector<ResLifetime> textureLifetimes;
    std::vector<ResLifetime> bufferLifetimes;
    void UpdateTextureLifetimeAndUsage( TGBTextureRef ref, TaskHandle task, VkImageUsageFlags flags = 0 );
    void UpdateBufferLifetime( TGBBufferRef ref, TaskHandle task );

    u16 numTasks;

private:
    // used during task graph compilation
    std::vector<ResourceTrackingInfo> texTracking;
    std::vector<ResourceTrackingInfo> bufTracking;
};

} // namespace PG::Gfx
