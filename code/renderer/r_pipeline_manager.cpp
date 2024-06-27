#include "r_pipeline_manager.hpp"
#include "asset/asset_manager.hpp"
#include "r_globals.hpp"
#include "renderer/debug_marker.hpp"
#include "renderer/graphics_api/pg_to_vulkan_types.hpp"
#include "shared/filesystem.hpp"

using namespace PG::Gfx;

static const char* s_pipelineCacheFilename = PG_ASSET_DIR "cache/pipeline_cache.bin";

#define PIPELINE_CACHE IN_USE
#define PIPELINE_STATS USE_IF( USING( DEVELOPMENT_BUILD ) )

#if USING( PIPELINE_STATS )
#include "core/time.hpp"
#define IF_PIPELINE_STATS( ... ) __VA_ARGS__
static f64 s_timeSpentBuildingPipelinesMS;
#else // #if USING( PIPELINE_STATS )
#define IF_PIPELINE_STATS( x ) \
    do                         \
    {                          \
    } while ( 0 )
#endif // #else // #if USING( PIPELINE_STATS )

namespace PG::Gfx::PipelineManager
{

static VkPipelineCache s_pipelineCache;

void Init()
{
    PGP_ZONE_SCOPEDN( "PipelineManager::Init" );
    IF_PIPELINE_STATS( s_timeSpentBuildingPipelinesMS = 0 );
    IF_PIPELINE_STATS( auto startTime = Time::GetTimePoint() );

    s_pipelineCache = VK_NULL_HANDLE;
#if USING( PIPELINE_CACHE )
    VkPipelineCacheCreateInfo pipelineCacheCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
    FileReadResult cacheBinary = ReadFile( s_pipelineCacheFilename );
    if ( cacheBinary )
    {
        pipelineCacheCreateInfo.initialDataSize = cacheBinary.size;
        pipelineCacheCreateInfo.pInitialData    = cacheBinary.data;
    }
    VK_CHECK( vkCreatePipelineCache( rg.device, &pipelineCacheCreateInfo, nullptr, &s_pipelineCache ) );
    cacheBinary.Free();
#endif // #if USING( PIPELINE_CACHE )

    IF_PIPELINE_STATS( LOG( "PipelineCache init time: %.2fms", Time::GetTimeSince( startTime ) ) );
}

void Shutdown()
{
#if USING( PIPELINE_CACHE )
    if ( s_pipelineCache != VK_NULL_HANDLE )
    {
        size_t size = 0;
        VK_CHECK( vkGetPipelineCacheData( rg.device, s_pipelineCache, &size, nullptr ) );

        std::vector<char> data( size );
        VK_CHECK( vkGetPipelineCacheData( rg.device, s_pipelineCache, &size, data.data() ) );

        if ( !WriteFile( s_pipelineCacheFilename, data.data(), data.size() ) )
            LOG_WARN( "Error while saving the pipeline cache" );

        vkDestroyPipelineCache( rg.device, s_pipelineCache, nullptr );
        s_pipelineCache = VK_NULL_HANDLE;
    }
#endif // #if USING( PIPELINE_CACHE )
    IF_PIPELINE_STATS( LOG( "Time spent building pipelines: %.2fms", s_timeSpentBuildingPipelinesMS ) );
}

class PipelineBuilder
{
public:
    void CreateGraphicsPipeline( Pipeline& p, const PipelineCreateInfo& createInfo )
    {
        p.SetName( createInfo.name );
        p.m_stageFlags    = 0;
        p.m_bindPoint     = VK_PIPELINE_BIND_POINT_GRAPHICS;
        p.m_pipelineType  = PipelineType::GRAPHICS;
        p.m_workgroupSize = uvec3( 0 );

        i32 taskShaderIdx          = -1;
        i32 meshShaderIdx          = -1;
        i32 pushConstantLowOffset  = INT16_MAX;
        i32 pushConstantHighOffset = 0;

        PG_ASSERT( createInfo.shaders.size() <= 3, "increase shaderStages size below" );
        VkPipelineShaderStageCreateInfo shaderStages[3];
        Shader* shaders[3];
        for ( i32 i = 0; i < (i32)createInfo.shaders.size(); ++i )
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
            pushConstantLowOffset          = Min<i32>( pushConstantLowOffset, rData.pushConstantOffset );
            pushConstantHighOffset         = Max<i32>( pushConstantHighOffset, rData.pushConstantOffset + rData.pushConstantSize );

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

        i32 pushConstantSize       = Max( 0, pushConstantHighOffset - pushConstantLowOffset );
        VkPushConstantRange pRange = { p.m_stageFlags, (u32)pushConstantLowOffset, (u32)pushConstantSize };

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
        colorBlending.attachmentCount = (u32)gInfo.colorAttachments.size();
        colorBlending.pAttachments    = colorBlendAttachment;

        VkPipelineDepthStencilStateCreateInfo depthStencil{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
        depthStencil.depthTestEnable       = gInfo.depthInfo.depthTestEnabled;
        depthStencil.depthWriteEnable      = gInfo.depthInfo.depthWriteEnabled;
        depthStencil.depthCompareOp        = PGToVulkanCompareFunction( gInfo.depthInfo.compareFunc );
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable     = VK_FALSE;

        VkPipelineVertexInputStateCreateInfo vertexInputStateCI{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
        inputAssembly.topology               = PGToVulkanPrimitiveType( gInfo.primitiveType );
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkGraphicsPipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
        pipelineInfo.stageCount          = (u32)createInfo.shaders.size();
        pipelineInfo.pStages             = shaderStages;
        pipelineInfo.pVertexInputState   = useMeshShading ? nullptr : &vertexInputStateCI; // assuming pull-style vertex fetching always
        pipelineInfo.pInputAssemblyState = useMeshShading ? nullptr : &inputAssembly;
        pipelineInfo.pViewportState      = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState   = &multisampling;
        pipelineInfo.pDepthStencilState  = &depthStencil;
        pipelineInfo.pColorBlendState    = &colorBlending;
        pipelineInfo.pDynamicState       = &dynamicState;
        pipelineInfo.layout              = p.m_pipelineLayout;

        VkPipelineRenderingCreateInfo dynRenderingCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR };
        dynRenderingCreateInfo.colorAttachmentCount    = (u32)gInfo.colorAttachments.size();
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

        VK_CHECK( vkCreateGraphicsPipelines( rg.device, s_pipelineCache, 1, &pipelineInfo, nullptr, &p.m_pipeline ) );

        PG_DEBUG_MARKER_SET_PIPELINE_LAYOUT_NAME( p.m_pipelineLayout, createInfo.name );
        PG_DEBUG_MARKER_SET_PIPELINE_NAME( p.m_pipeline, createInfo.name );
    }

    void CreateComputePipeline( Pipeline& p, const PipelineCreateInfo& createInfo )
    {
        p.SetName( createInfo.name );
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
        VK_CHECK( vkCreateComputePipelines( rg.device, s_pipelineCache, 1, &computePipelineCreateInfo, nullptr, &p.m_pipeline ) );
        PG_DEBUG_MARKER_SET_PIPELINE_LAYOUT_NAME( p.m_pipelineLayout, createInfo.name );
        PG_DEBUG_MARKER_SET_PIPELINE_NAME( p.m_pipeline, createInfo.name );
    }
};

void CreatePipeline( Pipeline* pipeline, const PipelineCreateInfo& createInfo )
{
    IF_PIPELINE_STATS( auto startTime = Time::GetTimePoint() );
    PipelineBuilder builder;
    if ( createInfo.shaders.size() == 1 && createInfo.shaders[0].stage == ShaderStage::COMPUTE )
    {
        builder.CreateComputePipeline( *pipeline, createInfo );
    }
    else
    {
        builder.CreateGraphicsPipeline( *pipeline, createInfo );
    }

    IF_PIPELINE_STATS( s_timeSpentBuildingPipelinesMS += Time::GetTimeSince( startTime ) );
}

Pipeline* GetPipeline( const std::string& name, bool debugPermuation )
{
#if USING( DEVELOPMENT_BUILD )
    if ( debugPermuation )
    {
        return AssetManager::Get<Pipeline>( name + "_debug" );
    }
    else
#endif // #if USING( DEVELOPMENT_BUILD )
    {
        return AssetManager::Get<Pipeline>( name );
    }
}

} // namespace PG::Gfx::PipelineManager
