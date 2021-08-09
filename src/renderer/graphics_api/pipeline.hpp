#pragma once

#include "renderer/graphics_api/render_pass.hpp"
#include "renderer/graphics_api/descriptor.hpp"
#include "renderer/graphics_api/limits.hpp"
#include "renderer/graphics_api/vertex_descriptor.hpp"
#include "renderer/vulkan.hpp"

namespace PG
{

struct Shader;

namespace Gfx
{

    enum class AccessFlags
    {
        INDIRECT_COMMAND_READ_BIT           = 0x00000001,
        INDEX_READ_BIT                      = 0x00000002,
        VERTEX_ATTRIBUTE_READ_BIT           = 0x00000004,
        UNIFORM_READ_BIT                    = 0x00000008,
        INPUT_ATTACHMENT_READ_BIT           = 0x00000010,
        SHADER_READ_BIT                     = 0x00000020,
        SHADER_WRITE_BIT                    = 0x00000040,
        COLOR_ATTACHMENT_READ_BIT           = 0x00000080,
        COLOR_ATTACHMENT_WRITE_BIT          = 0x00000100,
        DEPTH_STENCIL_ATTACHMENT_READ_BIT   = 0x00000200,
        DEPTH_STENCIL_ATTACHMENT_WRITE_BIT  = 0x00000400,
        TRANSFER_READ_BIT                   = 0x00000800,
        TRANSFER_WRITE_BIT                  = 0x00001000,
        HOST_READ_BIT                       = 0x00002000,
        HOST_WRITE_BIT                      = 0x00004000,
        MEMORY_READ_BIT                     = 0x00008000,
        MEMORY_WRITE_BIT                    = 0x00010000,
    };

    enum class PipelineStageFlags
    {
        TOP_OF_PIPE_BIT                     = 0x00000001,
        DRAW_INDIRECT_BIT                   = 0x00000002,
        VERTEX_INPUT_BIT                    = 0x00000004,
        VERTEX_SHADER_BIT                   = 0x00000008,
        TESSELLATION_CONTROL_SHADER_BIT     = 0x00000010,
        TESSELLATION_EVALUATION_SHADER_BIT  = 0x00000020,
        GEOMETRY_SHADER_BIT                 = 0x00000040,
        FRAGMENT_SHADER_BIT                 = 0x00000080,
        EARLY_FRAGMENT_TESTS_BIT            = 0x00000100,
        LATE_FRAGMENT_TESTS_BIT             = 0x00000200,
        COLOR_ATTACHMENT_OUTPUT_BIT         = 0x00000400,
        COMPUTE_SHADER_BIT                  = 0x00000800,
        TRANSFER_BIT                        = 0x00001000,
        BOTTOM_OF_PIPE_BIT                  = 0x00002000,
        HOST_BIT                            = 0x00004000,
        ALL_GRAPHICS_BIT                    = 0x00008000,
        ALL_COMMANDS_BIT                    = 0x00010000,
    };

    enum class CompareFunction
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
    };

    enum class BlendFactor
    {
        ZERO                    = 0,
        ONE                     = 1,
        SRC_COLOR               = 2,
        ONE_MINUS_SRC_COLOR     = 3,
        SRC_ALPHA               = 4,
        ONE_MINUS_SRC_ALPHA     = 5,
        DST_COLOR               = 6,
        ONE_MINUS_DST_COLOR     = 7,
        DST_ALPHA               = 8,
        ONE_MINUS_DST_ALPHA     = 9,
        SRC_ALPHA_SATURATE      = 10,

        NUM_BLEND_FACTORS
    };

    enum class BlendEquation
    {
        ADD                 = 0,
        SUBTRACT            = 1,
        REVERSE_SUBTRACT    = 2,
        MIN                 = 3,
        MAX                 = 4,

        NUM_BLEND_EQUATIONS
    };

    struct PipelineColorAttachmentInfo
    {
        BlendFactor srcColorBlendFactor  = BlendFactor::SRC_ALPHA;
        BlendFactor dstColorBlendFactor  = BlendFactor::ONE_MINUS_SRC_ALPHA;
        BlendFactor srcAlphaBlendFactor  = BlendFactor::SRC_ALPHA;
        BlendFactor dstAlphaBlendFactor  = BlendFactor::ONE_MINUS_SRC_ALPHA;
        BlendEquation colorBlendEquation = BlendEquation::ADD;
        BlendEquation alphaBlendEquation = BlendEquation::ADD;
        bool blendingEnabled = false;
    };

    enum class  WindingOrder
    {
        COUNTER_CLOCKWISE = 0,
        CLOCKWISE         = 1,

        NUM_WINDING_ORDER
    };

    enum class CullFace
    {
        NONE            = 0,
        FRONT           = 1,
        BACK            = 2,
        FRONT_AND_BACK  = 3,

        NUM_CULL_FACE
    };

    enum class PolygonMode
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

        float x = 0;
        float y = 0;
        float width;
        float height;
        float minDepth = 0.0f;
        float maxDepth = 1.0f;
    };

    Viewport FullScreenViewport( bool vulkanFlipViewport = true );

    struct Scissor
    {
        Scissor() = default;
        Scissor( int w, int h ) : width( w ), height( h ) {}

        int x = 0;
        int y = 0;
        int width;
        int height;
    };

    Scissor FullScreenScissor();

    enum class PrimitiveType
    {
        POINTS          = 0,

        LINES           = 1,
        LINE_STRIP      = 2,

        TRIANGLES       = 3,
        TRIANGLE_STRIP  = 4,
        TRIANGLE_FAN    = 5,

        NUM_PRIMITIVE_TYPE
    };

    struct PipelineDescriptor
    {
        std::array< Shader*, 3 > shaders = {};
        VertexInputDescriptor vertexDescriptor;
        RenderPass* renderPass;
        RasterizerInfo rasterizerInfo;
        PrimitiveType primitiveType = PrimitiveType::TRIANGLES;
        PipelineDepthInfo depthInfo;
        std::array< PipelineColorAttachmentInfo, 8 > colorAttachmentInfos;
    };

    struct PipelineResourceLayout
    {
        // uint32_t attributeMask = 0;
	    // uint32_t renderTargetMask = 0;
	    DescriptorSetLayout sets[PG_MAX_NUM_DESCRIPTOR_SETS] = {};
	    uint32_t bindingStages[PG_MAX_NUM_DESCRIPTOR_SETS][PG_MAX_NUM_BINDINGS_PER_SET] = {};
	    uint32_t setStages[PG_MAX_NUM_DESCRIPTOR_SETS] = {};
	    VkPushConstantRange pushConstantRange = {};
	    uint32_t descriptorSetMask = 0;
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
        const PipelineResourceLayout* GetResourceLayout() const;
        const DescriptorSetLayout* GetDescriptorSetLayout( int set ) const;

        operator bool() const;

    private:
        PipelineDescriptor m_desc;
        PipelineResourceLayout m_resourceLayout;
        VkPipeline m_pipeline             = VK_NULL_HANDLE;
        VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
        VkDevice m_device                 = VK_NULL_HANDLE;
        bool m_isCompute = false;
    };

} // namespace Gfx
} // namespace PG
