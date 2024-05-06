#pragma once

#include "core/pixel_formats.hpp"
#include "renderer/graphics_api/buffer.hpp"
#include "renderer/graphics_api/texture.hpp"
#include "renderer/r_globals.hpp"
#include <bitset>
#include <functional>
#include <utility>
#include <vector>

#define TG_DEBUG USE_IF( USING( DEBUG_BUILD ) )

#if USING( TG_DEBUG )
#define TG_DEBUG_ONLY( x ) x
#define TG_ASSERT( ... ) PG_ASSERT( __VA_ARGS__ )
#else // #if USING( TG_DEBUG )
#define TG_DEBUG_ONLY( x )
#define TG_ASSERT( ... )
#endif // #else // #if USING( TG_DEBUG )

namespace PG
{
class Scene;
}

namespace PG::Gfx
{

class CommandBuffer;
class Swapchain;
struct RenderTask;

enum class ResourceType : uint8_t
{
    NONE            = 0,
    TEXTURE         = ( 1 << 0 ),
    BUFFER          = ( 1 << 1 ),
    COLOR_ATTACH    = ( 1 << 2 ),
    DEPTH_ATTACH    = ( 1 << 3 ),
    STENCIL_ATTACH  = ( 1 << 4 ),
    SWAPCHAIN_IMAGE = ( 1 << 5 ),
};
PG_DEFINE_ENUM_OPS( ResourceType )

enum class ResourceState : uint8_t
{
    READ       = 0,
    WRITE      = 1,
    READ_WRITE = 2,

    COUNT
};

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
};

struct TaskHandle
{
    uint16_t index : 14;
    TaskType type : 2;

    TaskHandle( uint16_t inIndex, TaskType inType ) : index( inIndex ), type( inType ) {}
};

using ResourceHandle = uint16_t;

struct TGBTextureRef
{
    ResourceHandle index;

    bool operator==( const TGBTextureRef& o ) const { return index == o.index; }
};

struct TGBBufferRef
{
    ResourceHandle index;

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

struct TGExecuteData;
class ComputeTask;
using ComputeFunction = std::function<void( ComputeTask*, TGExecuteData* )>;

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

    void BlitTexture( TGBTextureRef dst, TGBTextureRef src ) { textureBlits.emplace_back( dst, src ); }

    std::vector<TGBTextureTransfer> textureBlits;
};

class TaskGraphBuilder
{
    friend class ComputeTaskBuilder;
    friend class TaskGraph;

public:
    TaskGraphBuilder();
    ~TaskGraphBuilder();
    ComputeTaskBuilder* AddComputeTask( const std::string& name );
    TransferTaskBuilder* AddTransferTask( const std::string& name );

    TGBTextureRef RegisterExternalTexture( const std::string& name, PixelFormat format, uint32_t width, uint32_t height, uint32_t depth,
        uint32_t arrayLayers, uint32_t mipLevels, ExtTextureFunc func );
    TGBBufferRef RegisterExternalBuffer(
        const std::string& name, BufferUsage bufferUsage, VmaMemoryUsage memoryUsage, size_t size, ExtBufferFunc func );

private:
    TGBTextureRef AddTexture( const std::string& name, PixelFormat format, uint32_t width, uint32_t height, uint32_t depth,
        uint32_t arrayLayers, uint32_t mipLevels, ExtTextureFunc func );
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
    void UpdateTextureLifetime( TGBTextureRef ref, TaskHandle task );
    void UpdateBufferLifetime( TGBBufferRef ref, TaskHandle task );

    uint16_t numTasks;
};

struct TaskGraphCompileInfo
{
    uint32_t sceneWidth;
    uint32_t sceneHeight;
    uint32_t displayWidth;
    uint32_t displayHeight;
    bool mergeResources = true;
    bool showStats      = true;
};

struct BufferClearSubTask
{
    ResourceHandle bufferHandle;
    uint32_t clearVal;
};

struct TextureClearSubTask
{
    ResourceHandle textureHandle;
    vec4 clearVal;
};

struct TGExecuteData
{
    Scene* scene;
    FrameData* frameData;
    CommandBuffer* cmdBuf;
    TaskGraph* taskGraph;

    std::vector<VkImageMemoryBarrier2> scratchImageBarriers;
    std::vector<VkBufferMemoryBarrier2> scratchBufferBarriers;
};

class Task
{
public:
    TG_DEBUG_ONLY( std::string debugName );
    std::vector<VkImageMemoryBarrier2> imageBarriers;
    std::vector<VkBufferMemoryBarrier2> bufferBarriers;

    virtual ~Task()                                    = default;
    virtual void Execute( TGExecuteData& executeData ) = 0;
    virtual void SubmitBarriers( TGExecuteData& executeData );
};

class ComputeTask : public Task
{
public:
    void Execute( TGExecuteData& data ) override;

    std::vector<BufferClearSubTask> bufferClears;
    std::vector<TextureClearSubTask> textureClears;
    std::vector<VkImageMemoryBarrier2> imageBarriersPreClears;

    std::vector<ResourceHandle> inputBuffers;
    std::vector<ResourceHandle> outputBuffers;
    std::vector<ResourceHandle> inputTextures;
    std::vector<ResourceHandle> outputTextures;

    ComputeFunction function;
};

struct TextureTransfer
{
    ResourceHandle dst;
    ResourceHandle src;
};

class TransferTask : public Task
{
public:
    void Execute( TGExecuteData& data ) override;

    std::vector<TextureTransfer> textureBlits;
};

class TaskGraph
{
public:
    bool Compile( TaskGraphBuilder& builder, TaskGraphCompileInfo& compileInfo );
    void Free();

    void Execute( TGExecuteData& data );

    // only to be used by tasks internally, during Execute()
    Buffer* GetBuffer( ResourceHandle handle );
    Texture* GetTexture( ResourceHandle handle );

private:
    std::vector<Task*> tasks;
    std::vector<Buffer> buffers;
    std::vector<Texture> textures;
    std::vector<VmaAllocation> vmaAllocations;
    std::vector<std::pair<ResourceHandle, ExtBufferFunc>> externalBuffers;
    std::vector<std::pair<ResourceHandle, ExtTextureFunc>> externalTextures;
};

} // namespace PG::Gfx
