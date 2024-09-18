#pragma once

#include "asset/types/shader.hpp"
#include "core/pixel_formats.hpp"

#if USING( GPU_STRUCTS )
#include "renderer/vulkan.hpp"
#endif // #if USING( GPU_STRUCTS )

namespace PG
{

namespace Gfx
{
enum class CompareFunction : u8
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

enum class BlendFactor : u8
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

enum class BlendEquation : u8
{
    ADD              = 0,
    SUBTRACT         = 1,
    REVERSE_SUBTRACT = 2,
    MIN              = 3,
    MAX              = 4,

    NUM_BLEND_EQUATIONS
};

enum class BlendMode : u8
{
    NONE        = 0,
    ADDITIVE    = 1,
    ALPHA_BLEND = 2,

    NUM_BLEND_MODES
};

#if USING( GPU_STRUCTS )
VkPipelineColorBlendAttachmentState ConvertPGBlendModeToVK( BlendMode blendMode );
#endif // #if USING( GPU_STRUCTS )

enum class WindingOrder : u8
{
    COUNTER_CLOCKWISE = 0,
    CLOCKWISE         = 1,

    NUM_WINDING_ORDER
};

enum class CullFace : u8
{
    NONE           = 0,
    FRONT          = 1,
    BACK           = 2,
    FRONT_AND_BACK = 3,

    NUM_CULL_FACE
};

enum class PolygonMode : u8
{
    FILL  = 0,
    LINE  = 1,
    POINT = 2,

    NUM_POLYGON_MODES
};

struct PipelineColorAttachmentInfo
{
    PipelineColorAttachmentInfo() = default;
    PipelineColorAttachmentInfo( PixelFormat inFmt ) : format( inFmt ) {}

    PixelFormat format  = PixelFormat::INVALID;
    BlendMode blendMode = BlendMode::NONE;
};

struct RasterizerInfo
{
    WindingOrder winding    = WindingOrder::COUNTER_CLOCKWISE;
    CullFace cullFace       = CullFace::BACK;
    PolygonMode polygonMode = PolygonMode::FILL;
    bool depthBiasEnable    = false;
};

struct PipelineDepthInfo
{
    bool depthTestEnabled       = true;
    bool depthWriteEnabled      = true;
    CompareFunction compareFunc = CompareFunction::LESS;
    PixelFormat format          = PixelFormat::INVALID;
};

enum class PrimitiveType : u8
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
    std::vector<PipelineColorAttachmentInfo> colorAttachments;
    RasterizerInfo rasterizerInfo;
    PipelineDepthInfo depthInfo;
    PrimitiveType primitiveType = PrimitiveType::TRIANGLES;
};

enum class PipelineType : u8
{
    NONE     = 0,
    GRAPHICS = 1,
    COMPUTE  = 2,

    TOTAL
};

} // namespace Gfx

struct PipelineShaderInfo
{
    // in asset .paf files, the filename is needed, but after that the actual asset name is needed
    std::string name;
    ShaderStage stage;
};

struct PipelineCreateInfo : public BaseAssetCreateInfo
{
    std::vector<PipelineShaderInfo> shaders;
    std::vector<std::string> defines;
    bool generateDebugPermutation;
    Gfx::GraphicsPipelineCreateInfo graphicsInfo;
};

namespace Gfx::PipelineManager
{
class PipelineBuilder;
}

class Pipeline : public BaseAsset
{
    friend Gfx::PipelineManager::PipelineBuilder;

public:
    Pipeline() = default;

    bool Load( const BaseAssetCreateInfo* baseInfo ) override;
    bool FastfileLoad( Serializer* serializer ) override;
    bool FastfileSave( Serializer* serializer ) const override;

#if USING( CONVERTER )
    std::vector<PipelineShaderInfo> shaderConvertInfos;
    PipelineCreateInfo createInfo; // just for serializing in the converter
#endif                             // #if USING( CONVERTER )

#if USING( GPU_DATA )
    void Free() override;
    VkPipeline GetHandle() const;
    VkPipelineLayout GetLayoutHandle() const;
    VkPipelineBindPoint GetPipelineBindPoint() const;
    VkShaderStageFlags GetPushConstantShaderStages() const;
    Gfx::PipelineType GetPipelineType() const;
    uvec3 GetWorkgroupSize() const;

    operator bool() const;
    operator VkPipeline() const;

private:
    VkPipeline m_pipeline             = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipelineBindPoint m_bindPoint;
    VkShaderStageFlags m_stageFlags = VK_SHADER_STAGE_ALL;
    Gfx::PipelineType m_pipelineType;
    uvec3 m_workgroupSize;
#endif // #if USING( GPU_DATA )
};

} // namespace PG
