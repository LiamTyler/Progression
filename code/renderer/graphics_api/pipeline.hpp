#pragma once

#include "renderer/graphics_api/descriptor.hpp"
#include "renderer/graphics_api/limits.hpp"
#include "renderer/graphics_api/vertex_descriptor.hpp"
#include "renderer/vulkan.hpp"

namespace PG
{

struct Shader;

namespace Gfx
{

enum class CompareFunction : uint8_t
{
    NEVER   = 0,
    LESS    = 1,
    EQUAL   = 2,
    LEQUAL  = 3,
    GREATER = 4,
    NEQUAL  = 5,
    GEQUAL  = 6,
    ALWAYS  = 7,

    NUM_COMPARE_FUNCTION
};

struct PipelineDepthInfo
{
    bool depthTestEnabled       = true;
    bool depthWriteEnabled      = true;
    CompareFunction compareFunc = CompareFunction::LESS;
    PixelFormat format          = PixelFormat::INVALID;
};

enum class BlendFactor : uint8_t
{
    ZERO                = 0,
    ONE                 = 1,
    SRC_COLOR           = 2,
    ONE_MINUS_SRC_COLOR = 3,
    SRC_ALPHA           = 4,
    ONE_MINUS_SRC_ALPHA = 5,
    DST_COLOR           = 6,
    ONE_MINUS_DST_COLOR = 7,
    DST_ALPHA           = 8,
    ONE_MINUS_DST_ALPHA = 9,
    SRC_ALPHA_SATURATE  = 10,

    NUM_BLEND_FACTORS
};

enum class BlendEquation : uint8_t
{
    ADD              = 0,
    SUBTRACT         = 1,
    REVERSE_SUBTRACT = 2,
    MIN              = 3,
    MAX              = 4,

    NUM_BLEND_EQUATIONS
};

struct alignas( 8 ) PipelineColorAttachmentInfo
{
    PipelineColorAttachmentInfo() = default;
    PipelineColorAttachmentInfo( PixelFormat inFmt ) : format( inFmt ) {}

    BlendFactor srcColorBlendFactor  = BlendFactor::SRC_ALPHA;
    BlendFactor dstColorBlendFactor  = BlendFactor::ONE_MINUS_SRC_ALPHA;
    BlendFactor srcAlphaBlendFactor  = BlendFactor::SRC_ALPHA;
    BlendFactor dstAlphaBlendFactor  = BlendFactor::ONE_MINUS_SRC_ALPHA;
    BlendEquation colorBlendEquation = BlendEquation::ADD;
    BlendEquation alphaBlendEquation = BlendEquation::ADD;
    bool blendingEnabled             = false;
    PixelFormat format               = PixelFormat::INVALID;
};

enum class WindingOrder : uint8_t
{
    COUNTER_CLOCKWISE = 0,
    CLOCKWISE         = 1,

    NUM_WINDING_ORDER
};

enum class CullFace : uint8_t
{
    NONE           = 0,
    FRONT          = 1,
    BACK           = 2,
    FRONT_AND_BACK = 3,

    NUM_CULL_FACE
};

enum class PolygonMode : uint8_t
{
    FILL  = 0,
    LINE  = 1,
    POINT = 2,

    NUM_POLYGON_MODES
};

struct RasterizerInfo
{
    WindingOrder winding    = WindingOrder::COUNTER_CLOCKWISE;
    CullFace cullFace       = CullFace::BACK;
    PolygonMode polygonMode = PolygonMode::FILL;
    bool depthBiasEnable    = false;
};

struct Viewport
{
    Viewport() = default;
    Viewport( float w, float h ) : width( w ), height( h ) {}

    float x        = 0;
    float y        = 0;
    float width    = 0;
    float height   = 0;
    float minDepth = 0.0f;
    float maxDepth = 1.0f;
};

struct Scissor
{
    Scissor() = default;
    Scissor( int w, int h ) : width( w ), height( h ) {}

    int x      = 0;
    int y      = 0;
    int width  = 0;
    int height = 0;
};

enum class PrimitiveType : uint8_t
{
    POINTS = 0,

    LINES      = 1,
    LINE_STRIP = 2,

    TRIANGLES      = 3,
    TRIANGLE_STRIP = 4,
    TRIANGLE_FAN   = 5,

    NUM_PRIMITIVE_TYPE
};

struct GraphicsPipelineCreateInfo
{
    std::vector<Shader*> shaders;
    std::vector<PipelineColorAttachmentInfo> colorAttachments;
    RasterizerInfo rasterizerInfo;
    PipelineDepthInfo depthInfo;
    PrimitiveType primitiveType = PrimitiveType::TRIANGLES;
};

enum class PipelineType : uint8_t
{
    NONE     = 0,
    GRAPHICS = 1,
    COMPUTE  = 2,

    TOTAL
};

class Pipeline
{
    friend class Device;

public:
    Pipeline() = default;

    void Free();
    VkPipeline GetHandle() const;
    VkPipelineLayout GetLayoutHandle() const;
    VkPipelineBindPoint GetPipelineBindPoint() const;
    VkShaderStageFlags GetPushConstantShaderStages() const;
    PipelineType GetPipelineType() const;
    uvec3 GetWorkgroupSize() const;

    operator bool() const;
    operator VkPipeline() const;

private:
    VkPipeline m_pipeline             = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipelineBindPoint m_bindPoint;
    VkShaderStageFlags m_stageFlags = VK_SHADER_STAGE_ALL;
    PipelineType m_pipelineType;
    uvec3 m_workgroupSize;
};

} // namespace Gfx
} // namespace PG
