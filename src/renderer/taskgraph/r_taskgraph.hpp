#pragma once

#include "core/pixel_formats.hpp"
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

    RESOURCETYPE_COUNT
};

enum class ResourceState : uint8_t
{
    READ_ONLY,
    READ_WRITE,
    WRITE_ONLY,

    RESOURCESTATE_COUNT
};

#define SCENE_WIDTH() (0xffff)
#define SCENE_HEIGHT() (0xffff)
#define SCENE_WIDTH_DIV( x ) ((x << 16) | 0xffff)
#define SCENE_HEIGHT_DIV( x ) ((x << 16) | 0xffff)
#define AUTO_FULL_MIP_CHAIN() (0xffff)

struct TG_ResourceDesc
{
    TG_ResourceDesc() = default;
    TG_ResourceDesc( PixelFormat inFormat, uint32_t inWidth, uint32_t inHeight, uint32_t inDepth, uint32_t inArrayLayers, uint32_t inMipLevels, const glm::vec4& inClearColor );
    
    void ResolveSizes( uint16_t sceneWidth, uint16_t sceneHeight );
    bool operator==( const TG_ResourceDesc& d ) const;
    bool Mergable( const TG_ResourceDesc& d ) const;

    PixelFormat format   = PixelFormat::INVALID;
    uint32_t width       = 0;
    uint32_t height      = 0;
    uint32_t depth       = 1;
    uint32_t arrayLayers = 1;
    uint32_t mipLevels   = 1;
    glm::vec4 clearColor = glm::vec4( 0 );
};

struct TG_ResourceInput
{
    TG_ResourceInput( ResourceType _type, const std::string& _name );

    ResourceType type;
    std::string name;
    uint16_t physicalResourceIndex;
};

struct TG_ResourceOutput
{
    ResourceType type;
    std::string name;
    TG_ResourceDesc desc;
    uint16_t physicalResourceIndex;
};

class RenderTaskBuilder
{
public:
    RenderTaskBuilder( const std::string& inName );

    void AddColorOutput( const std::string& name, PixelFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint32_t mipLevels, const glm::vec4& clearColor );
    void AddColorOutput( const std::string& name );
    void AddTextureInput( const std::string& name );
    void AddTextureInputOutput( const std::string& name );
    void AddTextureOutput( const std::string& name, PixelFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint32_t mipLevels, const glm::vec4& clearColor );
    void AddTextureOutput( const std::string& name );

    std::string name;
    std::string dependentPass;
    std::vector< TG_ResourceInput > inputs;
    std::vector< TG_ResourceOutput > outputs;
};

struct PhysicalGraphResource
{
    bool Mergable( const PhysicalGraphResource* res ) const;

    std::string name;
    TG_ResourceDesc desc;
    uint16_t firstTask;
    uint16_t lastTask;
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
    void PrintTaskGraph() const;

private:
    std::vector< RenderTaskBuilder > buildRenderTasks;

    static constexpr uint16_t MAX_FINAL_TASKS = 64;
    RenderTask renderTasks[MAX_FINAL_TASKS];
    uint16_t numRenderTasks;

    static constexpr uint16_t MAX_PHYSICAL_RESOURCES = 256;
    PhysicalGraphResource physicalResources[MAX_PHYSICAL_RESOURCES];
    uint16_t numPhysicalResources;

    uint16_t numLogicalOutputs;
};

} // namespace Gfx
} // namespace PG
