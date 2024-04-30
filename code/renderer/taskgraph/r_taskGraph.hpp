#pragma once

#include "core/pixel_formats.hpp"
#include "renderer/graphics_api/buffer.hpp"
#include "renderer/graphics_api/texture.hpp"
#include "renderer/r_globals.hpp"
#include <bitset>
#include <vector>

#define RG_DEBUG USE_IF( USING( DEBUG_BUILD ) )

#if USING( RG_DEBUG )
#define RG_DEBUG_ONLY( x ) x
#else // #if USING( RG_DEBUG )
#define RG_DEBUG_ONLY( x )
#endif // #else // #if USING( RG_DEBUG )

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
    READ_ONLY  = 0,
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

using ExtTextureFunc = VkImageView ( * )( void );
using ExtBufferFunc  = VkBufferView ( * )( void );

struct TGBTexture
{
    RG_DEBUG_ONLY( std::string debugName );
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
    RG_DEBUG_ONLY( std::string debugName );
    size_t size;
    BufferUsage bufferUsage;
    VmaMemoryUsage memoryUsage;
    ExtBufferFunc externalFunc;
};

enum class TaskType : uint16_t
{
    COMPUTE  = 0,
    GRAPHICS = 1,
    TRANSFER = 2,
};

struct TaskHandle
{
    uint16_t index : 14;
    TaskType type : 2;

    TaskHandle( uint16_t inIndex, TaskType inType ) : index( inIndex ), type( inType ) {}
};

using ResourceIndexHandle = uint16_t;

struct TGBTextureRef
{
    ResourceIndexHandle index;
};

struct TGBBufferRef
{
    ResourceIndexHandle index;
};

struct RGBTaskBufferClear
{
    uint32_t clearVal;
    TGBBufferRef ref;
    bool isCleared;
};

struct RGBTaskTextureClear
{
    vec4 clearColor;
    TGBTextureRef ref;
    bool isCleared;
};

using ComputeFunction = void ( * )( void );

class TaskGraphBuilder;

class ComputeTaskBuilder
{
public:
    ComputeTaskBuilder( TaskGraphBuilder* inBuilder, uint16_t taskIndex, const std::string& inName )
        : builder( inBuilder ), taskHandle( taskIndex, TaskType::COMPUTE )
#if USING( RG_DEBUG )
          ,
          debugName( inName )
#endif // #if USING( RG_DEBUG )
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

    RG_DEBUG_ONLY( std::string debugName );
    std::vector<RGBTaskBufferClear> buffers;
    std::vector<RGBTaskTextureClear> textures;
    ComputeFunction function;
    TaskGraphBuilder* const builder;
    const TaskHandle taskHandle;
};

class TaskGraphBuilder
{
    static constexpr uint32_t MAX_TASKS = 128;

    friend class ComputeTaskBuilder;
    friend class TaskGraph;

public:
    TaskGraphBuilder();
    ComputeTaskBuilder* AddComputeTask( const std::string& name );

    TGBTextureRef RegisterExternalTexture( const std::string& name, PixelFormat format, uint32_t width, uint32_t height, uint32_t depth,
        uint32_t arrayLayers, uint32_t mipLevels, ExtTextureFunc func );
    TGBBufferRef RegisterExternalBuffer(
        const std::string& name, BufferUsage bufferUsage, VmaMemoryUsage memoryUsage, size_t size, ExtBufferFunc func );

private:
    TGBTextureRef AddTexture( const std::string& name, PixelFormat format, uint32_t width, uint32_t height, uint32_t depth,
        uint32_t arrayLayers, uint32_t mipLevels, ExtTextureFunc func );
    TGBBufferRef AddBuffer( const std::string& name, BufferUsage bufferUsage, VmaMemoryUsage memoryUsage, size_t size, ExtBufferFunc func );

    std::vector<ComputeTaskBuilder> computeTasks;
    std::vector<TGBTexture> textures;
    std::vector<TGBBuffer> buffers;

    struct Accesses
    {
        uint16_t firstTask = UINT16_MAX;
        uint16_t lastTask  = 0;
    };
    std::vector<Accesses> textureAccesses;
    std::vector<Accesses> bufferAccesses;
    void MarkTextureRead( TGBTextureRef ref, TaskHandle task );
    void MarkTextureWrite( TGBTextureRef ref, TaskHandle task );
    void MarkBufferRead( TGBBufferRef ref, TaskHandle task );
    void MarkBufferWrite( TGBBufferRef ref, TaskHandle task );

    uint16_t taskIndex;
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

struct Task
{
    RG_DEBUG_ONLY( std::string debugName );
};

class TaskGraph
{
public:
    bool Compile( TaskGraphBuilder& builder, TaskGraphCompileInfo& compileInfo );
    void Free();

private:
    std::vector<Task> tasks;
    std::vector<Buffer> buffers;
    std::vector<Texture> textures;
    std::vector<VmaAllocation> vmaAllocations;
};

} // namespace PG::Gfx
