#pragma once

#include "core/pixel_formats.hpp"
#include "renderer/graphics_api/framebuffer.hpp"
#include "renderer/graphics_api/render_pass.hpp"
#include "renderer/graphics_api/texture.hpp"
#include "glm/vec4.hpp"
#include <functional>
#include <vector>

namespace PG
{

class Scene;

namespace Gfx
{

enum class ResourceType : uint8_t
{
    COLOR_ATTACH,
    DEPTH_ATTACH,
    TEXTURE,
    BUFFER,

    COUNT
};

enum class ResourceState : uint8_t
{
    READ_ONLY,
    WRITE,

    COUNT
};

enum class RelativeSizes : uint32_t
{
    Scene = 1 << 30,
    Display = 1 << 31,

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


struct RG_Element
{
    std::string name;
    ResourceType type    = ResourceType::COUNT;
    ResourceState state  = ResourceState::COUNT;
    PixelFormat format   = PixelFormat::INVALID;
    uint32_t width       = 0;
    uint32_t height      = 0;
    uint32_t depth       = 1;
    uint32_t arrayLayers = 1;
    uint32_t mipLevels   = 1;
    glm::vec4 clearColor = glm::vec4( 0 );
    bool isCleared       = false;
};

struct RG_PhysicalResource
{
    std::string name;
    union
    {
        Gfx::Texture texture;
    };
};

struct RG_TaskRenderTargets
{
    Texture* colorAttachments[MAX_COLOR_ATTACHMENTS];
    Texture* depthAttach;
    uint8_t numColorAttachments;
};

struct RenderTask
{
    std::string name;

    RenderPass renderPass;
    Framebuffer framebuffer;
    RenderFunction renderFunction;
    RG_TaskRenderTargets renderTargets;
};

struct RenderGraphCompileInfo
{
    uint32_t sceneWidth;
    uint32_t sceneHeight;
    uint32_t displayWidth;
    uint32_t displayHeight;
};

class CommandBuffer;
using RenderFunction = std::function<void( RenderTask*, Scene* scene, CommandBuffer* cmdBuf )>;

class RenderTaskBuilder
{
public:
    RenderTaskBuilder( const std::string& inName ) : name( inName ) {}

    void AddColorOutput( const std::string& name, PixelFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint32_t mipLevels, const glm::vec4& clearColor );
    void AddColorOutput( const std::string& name, PixelFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint32_t mipLevels );
    void AddColorOutput( const std::string& name );
    void AddDepthOutput( const std::string& name, PixelFormat format, uint32_t width, uint32_t height, float clearValue );
    void AddDepthOutput( const std::string& name, PixelFormat format, uint32_t width, uint32_t height );
    void AddDepthOutput( const std::string& name );
    void AddTextureOutput( const std::string& name, PixelFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint32_t mipLevels, const glm::vec4& clearColor );
    void AddTextureOutput( const std::string& name, PixelFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint32_t mipLevels );
    void AddTextureOutput( const std::string& name );
    void AddTextureInput( const std::string& name );

    void SetRenderFunction( RenderFunction func );

    std::string name;
    std::vector< RG_Element > elements;
    RenderFunction renderFunction;
};


class RenderGraphBuilder
{
    friend class RenderGraph;
public:
    RenderGraphBuilder();
    RenderTaskBuilder* AddTask( const std::string& name );

    bool Validate() const;

private:
    std::vector< RenderTaskBuilder > tasks;
};


class RenderGraph
{
public:
    RenderGraph() = default;

    bool Compile( RenderGraphBuilder& builder, RenderGraphCompileInfo& compileInfo );
    void Free();
    void PrintTaskGraph() const;

    void Render( Scene* scene, CommandBuffer* cmdBuf );

    static constexpr uint16_t MAX_TASKS = 64;
    static constexpr uint16_t MAX_PHYSICAL_RESOURCES = 256;

    struct Statistics
    {
        float memUsedMB;
        uint16_t numTextures;
        uint16_t numLogicalOutputs;
    };
    Statistics stats;

//private:
    bool AllocatePhysicalResources();

    RenderTask renderTasks[MAX_TASKS];
    uint16_t numRenderTasks;

    RG_PhysicalResource physicalResources[MAX_PHYSICAL_RESOURCES];
    uint16_t numPhysicalResources;
};

} // namespace Gfx
} // namespace PG
