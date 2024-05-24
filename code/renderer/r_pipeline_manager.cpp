#include "r_pipeline_manager.hpp"
#include "asset/asset_manager.hpp"
#include "r_globals.hpp"
#include "renderer/debug_marker.hpp"
#include "renderer/graphics_api/pg_to_vulkan_types.hpp"

using namespace PG::Gfx;

namespace PG::Gfx::PipelineManager
{

static VkPipelineCache s_pipelineCache;

void Init()
{
    VkPipelineCacheCreateInfo pipelineCacheCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
    VK_CHECK( vkCreatePipelineCache( rg.device, &pipelineCacheCreateInfo, nullptr, &s_pipelineCache ) );
}

void Shutdown() { vkDestroyPipelineCache( rg.device, s_pipelineCache, nullptr ); }

class PipelineBuilder
{
public:
    Pipeline CreateGraphicsPipeline( const PipelineCreateInfo& createInfo )
    {
        Pipeline p       = {};
        p.name           = createInfo.name;
        p.m_stageFlags   = 0;
        p.m_bindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
        p.m_pipelineType = PipelineType::GRAPHICS;

        int taskShaderIdx          = -1;
        int meshShaderIdx          = -1;
        int pushConstantLowOffset  = INT16_MAX;
        int pushConstantHighOffset = 0;

        PG_ASSERT( createInfo.shaders.size() <= 3, "increase shaderStages size below" );
        VkPipelineShaderStageCreateInfo shaderStages[3];
        Shader* shaders[3];
        for ( int i = 0; i < (int)createInfo.shaders.size(); ++i )
        {
            Shader* shader = AssetManager::Get<Shader>( createInfo.shaders[i].name );
            PG_ASSERT( shader && shader->GetShaderStage() == createInfo.shaders[i].stage );
            shaders[i]             = shader;
            ShaderStage stage      = shader->GetShaderStage();
            shaderStages[i]        = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
            shaderStages[i].stage  = PGToVulkanShaderStage( stage );
            shaderStages[i].module = shader->GetHandle();
            shaderStages[i].pName  = "main";
            p.m_stageFlags |= PGToVulkanShaderStage( stage );

            const ShaderReflectData& rData = shader->GetReflectionData();
            pushConstantLowOffset          = Min<int>( pushConstantLowOffset, rData.pushConstantOffset );
            pushConstantHighOffset         = Max<int>( pushConstantHighOffset, rData.pushConstantOffset + rData.pushConstantSize );

            if ( stage == ShaderStage::TASK )
                taskShaderIdx = i;
            if ( stage == ShaderStage::MESH )
                meshShaderIdx = i;
        }
        PG_ASSERT( !( taskShaderIdx > -1 && meshShaderIdx == -1 ), "If using a task shader, there must be a corresponding mesh shader" );
        bool useMeshShading = meshShaderIdx != -1;
        if ( useMeshShading )
            p.m_workgroupSize = shaders[taskShaderIdx > -1 ? taskShaderIdx : meshShaderIdx]->GetReflectionData().workgroupSize;
        else
            p.m_workgroupSize = uvec3( 0 );

        int pushConstantSize       = Max( 0, pushConstantHighOffset - pushConstantLowOffset );
        VkPushConstantRange pRange = { p.m_stageFlags, (uint32_t)pushConstantLowOffset, (uint32_t)pushConstantSize };

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        pipelineLayoutCreateInfo.setLayoutCount         = 1;
        pipelineLayoutCreateInfo.pSetLayouts            = &GetGlobalDescriptorSetLayout();
        pipelineLayoutCreateInfo.pushConstantRangeCount = pushConstantSize ? 1 : 0;
        pipelineLayoutCreateInfo.pPushConstantRanges    = pushConstantSize ? &pRange : nullptr;
        VK_CHECK( vkCreatePipelineLayout( rg.device, &pipelineLayoutCreateInfo, NULL, &p.m_pipelineLayout ) );

        VkDynamicState vkDynamicStates[2] = { VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_VIEWPORT };

        VkPipelineDynamicStateCreateInfo dynamicState{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
        dynamicState.dynamicStateCount = ARRAY_COUNT( vkDynamicStates );
        dynamicState.pDynamicStates    = vkDynamicStates;

        VkPipelineViewportStateCreateInfo viewportState{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
        viewportState.viewportCount = 1;
        viewportState.scissorCount  = 1;

        const GraphicsPipelineCreateInfo& gInfo = createInfo.graphicsInfo;
        VkPipelineRasterizationStateCreateInfo rasterizer{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
        rasterizer.depthClampEnable        = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode             = PGToVulkanPolygonMode( gInfo.rasterizerInfo.polygonMode );
        rasterizer.lineWidth               = 1.0f; // to be higher than 1 needs the wideLines GPU feature
        rasterizer.cullMode                = PGToVulkanCullFace( gInfo.rasterizerInfo.cullFace );
        rasterizer.frontFace               = PGToVulkanWindingOrder( gInfo.rasterizerInfo.winding );
        rasterizer.depthBiasEnable         = gInfo.rasterizerInfo.depthBiasEnable;

        VkPipelineMultisampleStateCreateInfo multisampling{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
        multisampling.sampleShadingEnable  = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        PG_ASSERT( gInfo.colorAttachments.size() <= 8, "update array sizes below" );
        VkPipelineColorBlendAttachmentState colorBlendAttachment[8];
        VkFormat colorAttachmentFormats[8];
        for ( size_t i = 0; i < gInfo.colorAttachments.size(); ++i )
        {
            const PipelineColorAttachmentInfo& attach = gInfo.colorAttachments[i];

            colorBlendAttachment[i] = {};
            colorBlendAttachment[i].colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            colorBlendAttachment[i].blendEnable = false;
            // colorBlendAttachment[i].blendEnable = attach.blendingEnabled;
            // if ( attach.blendingEnabled )
            //{
            //     colorBlendAttachment[i].srcColorBlendFactor = PGToVulkanBlendFactor( attach.srcColorBlendFactor );
            //     colorBlendAttachment[i].dstColorBlendFactor = PGToVulkanBlendFactor( attach.dstColorBlendFactor );
            //     colorBlendAttachment[i].srcAlphaBlendFactor = PGToVulkanBlendFactor( attach.srcAlphaBlendFactor );
            //     colorBlendAttachment[i].dstAlphaBlendFactor = PGToVulkanBlendFactor( attach.dstAlphaBlendFactor );
            //     colorBlendAttachment[i].colorBlendOp        = PGToVulkanBlendEquation( attach.colorBlendEquation );
            //     colorBlendAttachment[i].alphaBlendOp        = PGToVulkanBlendEquation( attach.alphaBlendEquation );
            // }
            colorAttachmentFormats[i] = PGToVulkanPixelFormat( attach.format );
        }

        VkPipelineColorBlendStateCreateInfo colorBlending{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
        colorBlending.logicOpEnable   = VK_FALSE;
        colorBlending.logicOp         = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = (uint32_t)gInfo.colorAttachments.size();
        colorBlending.pAttachments    = colorBlendAttachment;

        VkPipelineDepthStencilStateCreateInfo depthStencil{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
        depthStencil.depthTestEnable       = gInfo.depthInfo.depthTestEnabled;
        depthStencil.depthWriteEnable      = gInfo.depthInfo.depthWriteEnabled;
        depthStencil.depthCompareOp        = PGToVulkanCompareFunction( gInfo.depthInfo.compareFunc );
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable     = VK_FALSE;

        VkGraphicsPipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
        pipelineInfo.stageCount          = (uint32_t)createInfo.shaders.size();
        pipelineInfo.pStages             = shaderStages;
        pipelineInfo.pVertexInputState   = nullptr; // assuming pull-style vertex fetching always
        pipelineInfo.pInputAssemblyState = nullptr;
        pipelineInfo.pViewportState      = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState   = &multisampling;
        pipelineInfo.pDepthStencilState  = &depthStencil;
        pipelineInfo.pColorBlendState    = &colorBlending;
        pipelineInfo.pDynamicState       = &dynamicState;
        pipelineInfo.layout              = p.m_pipelineLayout;

        VkFormat colorFmt = PGToVulkanPixelFormat( PixelFormat::R16_G16_B16_A16_FLOAT );
        VkPipelineRenderingCreateInfo dynRenderingCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR };
        dynRenderingCreateInfo.colorAttachmentCount    = (uint32_t)gInfo.colorAttachments.size();
        dynRenderingCreateInfo.pColorAttachmentFormats = colorAttachmentFormats;

        PixelFormat depthFmt = PixelFormat::INVALID;
        if ( gInfo.depthInfo.format != PixelFormat::INVALID )
        {
            VkFormat fmt                                 = PGToVulkanPixelFormat( gInfo.depthInfo.format );
            dynRenderingCreateInfo.depthAttachmentFormat = fmt;
            if ( PixelFormatHasStencil( gInfo.depthInfo.format ) )
                dynRenderingCreateInfo.stencilAttachmentFormat = fmt;
        }

        pipelineInfo.pNext = &dynRenderingCreateInfo;

        VK_CHECK( vkCreateGraphicsPipelines( rg.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &p.m_pipeline ) );

        PG_DEBUG_MARKER_SET_PIPELINE_LAYOUT_NAME( p.m_pipelineLayout, createInfo.name );
        PG_DEBUG_MARKER_SET_PIPELINE_NAME( p.m_pipeline, createInfo.name );

        return p;
    }

    Pipeline CreateComputePipeline( const PipelineCreateInfo& createInfo )
    {
        Pipeline p       = {};
        p.name           = createInfo.name;
        p.m_stageFlags   = VK_SHADER_STAGE_COMPUTE_BIT;
        p.m_bindPoint    = VK_PIPELINE_BIND_POINT_COMPUTE;
        p.m_pipelineType = PipelineType::COMPUTE;
        Shader* shader   = AssetManager::Get<Shader>( createInfo.shaders[0].name );
        PG_ASSERT( shader && shader->GetShaderStage() == createInfo.shaders[0].stage );
        p.m_workgroupSize = shader->GetReflectionData().workgroupSize;

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        pipelineLayoutCreateInfo.setLayoutCount = 1;
        pipelineLayoutCreateInfo.pSetLayouts    = &GetGlobalDescriptorSetLayout();
        VkPushConstantRange pRange              = {
            VK_SHADER_STAGE_COMPUTE_BIT, shader->GetReflectionData().pushConstantOffset, shader->GetReflectionData().pushConstantSize };
        pipelineLayoutCreateInfo.pushConstantRangeCount = pRange.size ? 1 : 0;
        pipelineLayoutCreateInfo.pPushConstantRanges    = pRange.size ? &pRange : nullptr;
        VK_CHECK( vkCreatePipelineLayout( rg.device, &pipelineLayoutCreateInfo, NULL, &p.m_pipelineLayout ) );

        VkPipelineShaderStageCreateInfo stageinfo{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        stageinfo.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
        stageinfo.module = shader->GetHandle();
        stageinfo.pName  = "main";

        VkComputePipelineCreateInfo computePipelineCreateInfo{ VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
#if USING( PG_DESCRIPTOR_BUFFER )
        computePipelineCreateInfo.flags |= VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
#endif // #if USING( PG_DESCRIPTOR_BUFFER )
        computePipelineCreateInfo.layout = p.GetLayoutHandle();
        computePipelineCreateInfo.stage  = stageinfo;
        VK_CHECK( vkCreateComputePipelines( rg.device, nullptr, 1, &computePipelineCreateInfo, nullptr, &p.m_pipeline ) );
        PG_DEBUG_MARKER_SET_PIPELINE_LAYOUT_NAME( p.m_pipelineLayout, createInfo.name );
        PG_DEBUG_MARKER_SET_PIPELINE_NAME( p.m_pipeline, createInfo.name );

        return p;
    }
};

Pipeline CreatePipeline( const PipelineCreateInfo& createInfo )
{
    PipelineBuilder builder;
    if ( createInfo.shaders.size() == 1 && createInfo.shaders[0].stage == ShaderStage::COMPUTE )
    {
        return builder.CreateComputePipeline( createInfo );
    }
    else
    {
        return builder.CreateGraphicsPipeline( createInfo );
    }
}

Pipeline* GetPipeline( const std::string& name, bool debugPermuation )
{
    if ( debugPermuation )
    {
        return AssetManager::Get<Pipeline>( name + "_debug" );
    }
    else
    {
        return AssetManager::Get<Pipeline>( name );
    }
}

} // namespace PG::Gfx::PipelineManager
