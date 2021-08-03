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

    COUNT
};

#define SCENE_WIDTH() (0xFFFF)
#define SCENE_HEIGHT() (0xFFFF)
#define SCENE_WIDTH_DIV( x ) ((x << 16) | 0xFFFF)
#define SCENE_HEIGHT_DIV( x ) ((x << 16) | 0xFFFF)
//#define DISPLAY_WIDTH() (0xFFFE)
//#define DISPLAY_HEIGHT() (0xFFFE)
//#define DISPLAY_WIDTH_DIV( x ) ((x << 16) | 0xFFFE)
//#define DISPLAY_HEIGHT_DIV( x ) ((x << 16) | 0xFFFE)
#define AUTO_FULL_MIP_CHAIN() (0xFFFF)

struct TG_ResourceDesc
{
    TG_ResourceDesc() = default;
    TG_ResourceDesc( ResourceType inType, PixelFormat inFormat, uint32_t inWidth, uint32_t inHeight, uint32_t inDepth, uint32_t inArrayLayers, uint32_t inMipLevels, const glm::vec4& inClearColor, bool inCleared );
    
    void ResolveSizes( uint16_t sceneWidth, uint16_t sceneHeight );
    bool operator==( const TG_ResourceDesc& d ) const;
    bool Mergable( const TG_ResourceDesc& d ) const;

    ResourceType type    = ResourceType::COUNT;
    PixelFormat format   = PixelFormat::INVALID;
    uint32_t width       = 0;
    uint32_t height      = 0;
    uint32_t depth       = 1;
    uint32_t arrayLayers = 1;
    uint32_t mipLevels   = 1;
    glm::vec4 clearColor = glm::vec4( 0 );
    bool isCleared       = false;
};

struct TG_ResourceInput
{
    std::string name;
    uint16_t createInfoIdx;
};

struct TG_ResourceOutput
{
    std::string name;
    TG_ResourceDesc desc;
    uint16_t createInfoIdx;
    uint8_t renderPassAttachmentIdx;
};

struct RenderTask;
class CommandBuffer;
using RenderFunction = std::function<void( RenderTask*, Scene* scene, CommandBuffer* cmdBuf )>;

class RenderTaskBuilder
{
public:
    RenderTaskBuilder( const std::string& inName );

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
    void AddTextureInputOutput( const std::string& name );
    void SetRenderFunction( RenderFunction func );

    std::string name;
    std::string dependentPass;
    std::vector< TG_ResourceInput > inputs;
    std::vector< TG_ResourceOutput > outputs;
    RenderFunction renderFunction;
};

class RenderGraphBuilder
{
    friend class RenderGraph;
public:
    RenderGraphBuilder();
    RenderTaskBuilder* AddTask( const std::string& name );

private:
    std::vector< RenderTaskBuilder > tasks;
};


struct GraphResource
{
    GraphResource() = default;
    GraphResource( const std::string& inName, const TG_ResourceDesc& inDesc, uint16_t currentTask );
    bool Mergable( const GraphResource& res ) const;

    std::string name;
    TG_ResourceDesc desc;
    uint16_t firstTask;
    uint16_t lastTask;
    uint16_t physicalResourceIndex;
};

struct RenderTask
{
    std::string name;
    static constexpr uint8_t MAX_INPUTS = 16;
    static constexpr uint8_t MAX_OUTPUTS = 16;
    uint16_t inputIndices[MAX_INPUTS];
    uint16_t outputIndices[MAX_OUTPUTS];
    uint8_t numInputs;
    uint8_t numOutputs;

    RenderPass renderPass;
    Framebuffer framebuffer;
    RenderFunction renderFunction;
};

class RenderGraph
{
public:
    RenderGraph() = default;

    bool Compile( RenderGraphBuilder& builder, uint16_t sceneWidth, uint16_t sceneHeight );
    void Free();
    void PrintTaskGraph() const;

    void Render( Scene* scene, CommandBuffer* cmdBuf );

    static constexpr uint16_t MAX_FINAL_TASKS = 64;
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

    RenderTask renderTasks[MAX_FINAL_TASKS];
    uint16_t numRenderTasks;

    GraphResource physicalResources[MAX_PHYSICAL_RESOURCES];
    Gfx::Texture textures[MAX_PHYSICAL_RESOURCES];
    uint16_t numPhysicalResources;
};

} // namespace Gfx
} // namespace PG
