#pragma once

#include "core/pixel_formats.hpp"
#include "renderer/graphics_api/texture.hpp"
#include "glm/vec4.hpp"
#include <vector>

namespace PG
{
namespace Gfx
{

enum class ResourceType : uint8_t
{
    COLOR_ATTACH,
    DEPTH_ATTACH,
    TEXTURE,

    COUNT
};

enum class ResourceState : uint8_t
{
    READ_ONLY,
    READ_WRITE,
    WRITE_ONLY,

    COUNT
};

#define SCENE_WIDTH() (0xffff)
#define SCENE_HEIGHT() (0xffff)
#define SCENE_WIDTH_DIV( x ) ((x << 16) | 0xffff)
#define SCENE_HEIGHT_DIV( x ) ((x << 16) | 0xffff)
#define AUTO_FULL_MIP_CHAIN() (0xffff)

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
};

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

    std::string name;
    std::string dependentPass;
    std::vector< TG_ResourceInput > inputs;
    std::vector< TG_ResourceOutput > outputs;
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
};

class RenderGraph
{
public:
    RenderGraph();

    RenderTaskBuilder* AddTask( const std::string& name );
    bool Compile( uint16_t sceneWidth, uint16_t sceneHeight );
    void Free();
    void PrintTaskGraph() const;

    static constexpr uint16_t MAX_FINAL_TASKS = 64;
    static constexpr uint16_t MAX_PHYSICAL_RESOURCES = 256;

private:
    bool AllocatePhysicalResources();
    bool CreateRenderPasses();

    std::vector< RenderTaskBuilder > buildRenderTasks;

    RenderTask renderTasks[MAX_FINAL_TASKS];
    uint16_t numRenderTasks;

    GraphResource physicalResources[MAX_PHYSICAL_RESOURCES];
    Gfx::Texture textures[MAX_PHYSICAL_RESOURCES];
    uint16_t numPhysicalResources;

    uint16_t numLogicalOutputs;
};

} // namespace Gfx
} // namespace PG
