#pragma once

#include "r_tg_tasks.hpp"
#include "renderer/graphics_api/buffer.hpp"
#include <functional>

namespace PG::Gfx
{

enum class RelativeSizes : uint32_t
{
    Scene   = 1u << 30,
    Display = 1u << 31,

    ALL = Scene | Display
};

constexpr uint32_t AUTO_FULL_MIP_CHAIN() { return UINT32_MAX; }
constexpr uint32_t SIZE_SCENE() { return static_cast<uint32_t>( RelativeSizes::Scene ); }
constexpr uint32_t SIZE_DISPLAY() { return static_cast<uint32_t>( RelativeSizes::Display ); }
constexpr uint32_t SIZE_SCENE_DIV( uint32_t x ) { return static_cast<uint32_t>( RelativeSizes::Scene ) | x; }
constexpr uint32_t SIZE_DISPLAY_DIV( uint32_t x ) { return static_cast<uint32_t>( RelativeSizes::Display ) | x; }
constexpr uint32_t ResolveRelativeSize( uint32_t scene, uint32_t display, uint32_t relSize )
{
    uint32_t size = relSize & ~(uint32_t)RelativeSizes::ALL;
    if ( relSize & (uint32_t)RelativeSizes::Scene )
    {
        return size == 0 ? scene : scene / size;
    }
    else if ( relSize & (uint32_t)RelativeSizes::Display )
    {
        return size == 0 ? display : display / size;
    }
    else
    {
        return relSize;
    }
}

using ExtTextureFunc = std::function<void( VkImage&, VkImageView& )>;
using ExtBufferFunc  = std::function<VkBuffer()>;

struct TGBTexture
{
    TG_DEBUG_ONLY( std::string debugName );
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint8_t arrayLayers;
    uint8_t mipLevels;
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

enum class TaskType : uint16_t
{
    NONE     = 0, // only valid internally for signaling a resource has no previous task yet (first usage)
    COMPUTE  = 1,
    GRAPHICS = 2,
    TRANSFER = 3,
    PRESENT  = 4,
};

struct TaskHandle
{
    uint16_t index : 13;
    TaskType type : 3;

    TaskHandle( uint16_t inIndex, TaskType inType ) : index( inIndex ), type( inType ) {}
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
    uint32_t clearVal;
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
    TaskBuilder( TaskGraphBuilder* inBuilder, TaskHandle inTaskHandle, const std::string& inName )
        : builder( inBuilder ), taskHandle( inTaskHandle )
#if USING( TG_DEBUG )
          ,
          debugName( inName )
#endif // #if USING( TG_DEBUG )
    {
    }

    virtual ~TaskBuilder() = default;

    TG_DEBUG_ONLY( std::string debugName );
    TaskGraphBuilder* const builder;
    const TaskHandle taskHandle;
};

class ComputeTaskBuilder : public TaskBuilder
{
public:
    ComputeTaskBuilder( TaskGraphBuilder* inBuilder, uint16_t taskIndex, const std::string& inName )
        : TaskBuilder( inBuilder, TaskHandle( taskIndex, TaskType::COMPUTE ), inName )
    {
    }

    TGBTextureRef AddTextureOutput( const std::string& name, PixelFormat format, const vec4& clearColor, uint32_t width, uint32_t height,
        uint32_t depth = 1, uint32_t arrayLayers = 1, uint32_t mipLevels = 1 );
    TGBTextureRef AddTextureOutput( const std::string& name, PixelFormat format, uint32_t width, uint32_t height, uint32_t depth = 1,
        uint32_t arrayLayers = 1, uint32_t mipLevels = 1 );
    void AddTextureOutput( TGBTextureRef& texture );
    void AddTextureInput( TGBTextureRef& texture );

    TGBBufferRef AddBufferOutput(
        const std::string& name, BufferUsage bufferUsage, VmaMemoryUsage memoryUsage, size_t size, uint32_t clearVal );
    TGBBufferRef AddBufferOutput( const std::string& name, BufferUsage bufferUsage, VmaMemoryUsage memoryUsage, size_t size );
    void AddBufferOutput( TGBBufferRef& buffer );
    void AddBufferInput( TGBBufferRef& buffer );

    void SetFunction( ComputeFunction func );

    std::vector<TGBBufferInfo> buffers;
    std::vector<TGBTextureInfo> textures;
    ComputeFunction function;
};

struct TGBAttachmentInfo
{
    vec4 clearColor;
    TGBTextureRef ref;
    ResourceType type;
    bool isCleared;
};

class GraphicsTaskBuilder : public TaskBuilder
{
public:
    GraphicsTaskBuilder( TaskGraphBuilder* inBuilder, uint16_t taskIndex, const std::string& inName )
        : TaskBuilder( inBuilder, TaskHandle( taskIndex, TaskType::GRAPHICS ), inName )
    {
    }

    TGBTextureRef AddColorAttachment( const std::string& name, PixelFormat format, const vec4& clearColor, uint32_t width, uint32_t height,
        uint32_t depth = 1, uint32_t arrayLayers = 1, uint32_t mipLevels = 1 );
    TGBTextureRef AddColorAttachment( const std::string& name, PixelFormat format, uint32_t width, uint32_t height, uint32_t depth = 1,
        uint32_t arrayLayers = 1, uint32_t mipLevels = 1 );
    void AddColorAttachment( TGBTextureRef tex );

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
    TransferTaskBuilder( TaskGraphBuilder* inBuilder, uint16_t taskIndex, const std::string& inName )
        : TaskBuilder( inBuilder, TaskHandle( taskIndex, TaskType::TRANSFER ), inName )
    {
    }

    void BlitTexture( TGBTextureRef dst, TGBTextureRef src );

    std::vector<TGBTextureTransfer> textureBlits;
};

class PresentTaskBuilder : public TaskBuilder
{
public:
    PresentTaskBuilder( TaskGraphBuilder* inBuilder, uint16_t taskIndex, const std::string& inName )
        : TaskBuilder( inBuilder, TaskHandle( taskIndex, TaskType::PRESENT ), inName )
    {
    }

    void SetPresentationImage( TGBTextureRef tex );

    TGBTextureRef presentationTex;
};

class TaskGraphBuilder
{
    friend class ComputeTaskBuilder;
    friend class GraphicsTaskBuilder;
    friend class TransferTaskBuilder;
    friend class PresentTaskBuilder;
    friend class TaskGraph;

public:
    TaskGraphBuilder();
    ~TaskGraphBuilder();
    ComputeTaskBuilder* AddComputeTask( const std::string& name );
    GraphicsTaskBuilder* AddGraphicsTask( const std::string& name );
    TransferTaskBuilder* AddTransferTask( const std::string& name );
    PresentTaskBuilder* AddPresentTask();

    TGBTextureRef RegisterExternalTexture( const std::string& name, PixelFormat format, uint32_t width, uint32_t height, uint32_t depth,
        uint32_t arrayLayers, uint32_t mipLevels, ExtTextureFunc func );
    TGBBufferRef RegisterExternalBuffer(
        const std::string& name, BufferUsage bufferUsage, VmaMemoryUsage memoryUsage, size_t size, ExtBufferFunc func );

private:
    TGBTextureRef AddTexture( const std::string& name, PixelFormat format, uint32_t width, uint32_t height, uint32_t depth,
        uint32_t arrayLayers, uint32_t mipLevels, ExtTextureFunc func, VkImageUsageFlags usage );
    TGBBufferRef AddBuffer( const std::string& name, BufferUsage bufferUsage, VmaMemoryUsage memoryUsage, size_t size, ExtBufferFunc func );

    std::vector<TaskBuilder*> tasks;
    std::vector<TGBTexture> textures;
    std::vector<TGBBuffer> buffers;

    struct ResLifetime
    {
        uint16_t firstTask = UINT16_MAX;
        uint16_t lastTask  = 0;
    };
    std::vector<ResLifetime> textureLifetimes;
    std::vector<ResLifetime> bufferLifetimes;
    void UpdateTextureLifetimeAndUsage( TGBTextureRef ref, TaskHandle task, VkImageUsageFlags flags = 0 );
    void UpdateBufferLifetime( TGBBufferRef ref, TaskHandle task );

    uint16_t numTasks;
};

} // namespace PG::Gfx