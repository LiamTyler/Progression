#pragma once

#include "core/pixel_formats.hpp"
#include "glm/vec4.hpp"
#include "renderer/graphics_api/framebuffer.hpp"
#include "renderer/r_globals.hpp"
#include "renderer/graphics_api/render_pass.hpp"
#include "renderer/graphics_api/texture.hpp"
#include <functional>
#include <vector>

namespace PG
{

class Scene;

namespace Gfx
{

class CommandBuffer;
class Swapchain;
struct RenderTask;

enum class ResourceType : uint8_t
{
    NONE = 0,
    TEXTURE = (1 << 0),
    BUFFER = (1 << 1),
    COLOR_ATTACH = (1 << 2),
    DEPTH_ATTACH = (1 << 3),
    STENCIL_ATTACH = (1 << 4),
    SWAPCHAIN_IMAGE = (1 << 5),
};
PG_DEFINE_ENUM_OPS( ResourceType )

enum class ResourceState : uint8_t
{
    READ_ONLY = 0,
    WRITE = 1,

    COUNT
};

enum class RelativeSizes : uint32_t
{
    Scene = 1u << 30,
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

struct RG_Element
{
    std::string name;
    ResourceType type    = ResourceType::NONE;
    ResourceState state  = ResourceState::COUNT;
    PixelFormat format   = PixelFormat::INVALID;
    uint32_t width       = 0;
    uint32_t height      = 0;
    uint32_t depth       = 1;
    uint32_t arrayLayers = 1;
    uint32_t mipLevels   = 1;
    glm::vec4 clearColor = glm::vec4( 0 );
    bool isCleared       = false;
    bool isExternal      = false;
};


struct RG_AttachmentInfo
{
    glm::vec4 clearColor;
    LoadAction loadAction;
    StoreAction storeAction;
    PixelFormat format;
    ImageLayout imageLayout;
    uint16_t physicalResourceIndex;
};

struct RG_TaskRenderTargetsDynamic
{
    RG_TaskRenderTargetsDynamic() : numColorAttach( 0 ), renderAreaWidth( USHRT_MAX ), renderAreaHeight( USHRT_MAX )
    {
        depthAttachInfo.format = PixelFormat::INVALID;
    }

    RG_AttachmentInfo colorAttachInfo[MAX_COLOR_ATTACHMENTS];
    RG_AttachmentInfo depthAttachInfo;
    uint8_t numColorAttach;
    uint16_t renderAreaWidth;
    uint16_t renderAreaHeight;

    void AddColorAttach( const RG_Element& element, uint16_t phyRes, LoadAction loadAction, StoreAction storeAction );
    void AddDepthAttach( const RG_Element& element, uint16_t phyRes, LoadAction loadAction, StoreAction storeAction );
};

struct RG_RenderData
{
    Scene* scene;
    CommandBuffer* cmdBuf;
    Swapchain* swapchain;
    uint32_t swapchainImageIndex;
};

using RenderFunction = std::function<void( RenderTask* task, RG_RenderData& renderData )>;

struct RenderTask
{
    struct ImageTransition
    {
        ImageLayout previousLayout;
        ImageLayout desiredLayout;
        uint16_t physicalResourceIndex;
        VkImageAspectFlags aspect;
    };

    void AddTransition( ImageLayout previousLayout, ImageLayout desiredLayout, uint16_t physicalResourceIndex, ResourceType resType )
    {
        VkImageAspectFlags aspect = VK_IMAGE_ASPECT_NONE;
        if ( IsSet( resType, ResourceType::COLOR_ATTACH ) )
        {
            aspect = VK_IMAGE_ASPECT_COLOR_BIT;
        }
        else
        {
            if ( IsSet( resType, ResourceType::DEPTH_ATTACH ) ) aspect |= VK_IMAGE_ASPECT_DEPTH_BIT;
            if ( IsSet( resType, ResourceType::STENCIL_ATTACH ) ) aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
        imageTransitions.emplace_back( previousLayout, desiredLayout, physicalResourceIndex, aspect );
    }

    std::string name;
    RenderFunction renderFunction;
    RG_TaskRenderTargetsDynamic* renderTargetData = nullptr;
    std::vector<ImageTransition> imageTransitions;
};


struct RenderGraphCompileInfo
{
    uint32_t sceneWidth;
    uint32_t sceneHeight;
    uint32_t displayWidth;
    uint32_t displayHeight;
    PixelFormat swapchainFormat;
};


class RenderTaskBuilder
{
public:
    RenderTaskBuilder( const std::string& inName ) : name( inName ) {}

    void AddColorOutput( const std::string& name, PixelFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint32_t mipLevels, const glm::vec4& clearColor );
    void AddColorOutput( const std::string& name, PixelFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint32_t mipLevels );
    void AddColorOutput( const std::string& name );
    void AddSwapChainOutput( const glm::vec4& clearColor );
    void AddSwapChainOutput();
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

    bool Validate( const RenderGraphCompileInfo& compileInfo ) const;

private:
    std::vector< RenderTaskBuilder > tasks;
};


struct RG_PhysicalResource
{
    RG_PhysicalResource() {}
    ~RG_PhysicalResource() {}

    std::string name;
    union
    {
        Gfx::Texture texture;
    };
    uint16_t firstTask;
    uint16_t lastTask;
    bool isExternal = false;
};


class RenderGraph
{
public:
    RenderGraph() {}

    bool Compile( RenderGraphBuilder& builder, RenderGraphCompileInfo& compileInfo );
    void Free();
    void Print() const;
    void Render( RG_RenderData& renderData );

    Texture* GetTexture( uint16_t idx );
    RG_PhysicalResource* GetPhysicalResource( uint16_t idx );
    RG_PhysicalResource* GetPhysicalResource( const std::string& logicalName, uint8_t frameInFlight );
    RenderTask* GetRenderTask( const std::string& name );
    PipelineAttachmentInfo GetPipelineAttachmentInfo( const std::string& name ) const;

    struct Statistics
    {
        float memUsedMB;
        uint16_t numTextures;
        uint16_t numLogicalOutputs;
    };
    Statistics GetStats() const;

    static constexpr uint16_t MAX_TASKS = 64;
    static constexpr uint16_t MAX_PHYSICAL_RESOURCES_PER_FRAME = 256;

private:
    uint8_t frameInFlight = 0;
    uint16_t numRenderTasks;
    RenderTask renderTasks[MAX_TASKS];
    std::unordered_map<std::string, uint16_t> taskNameToIndexMap;

    uint16_t numPhysicalResources;
    RG_PhysicalResource physicalResources[MAX_PHYSICAL_RESOURCES_PER_FRAME][MAX_FRAMES_IN_FLIGHT];
    uint16_t swapchainPhysicalResIdx;
    std::unordered_map<std::string, uint16_t> resourceNameToIndexMap;

    Statistics stats;
};

} // namespace Gfx
} // namespace PG
