#include "render_system.hpp"
#include "asset/asset_manager.hpp"
#include "core/engine_globals.hpp"
#include "core/scene.hpp"
#include "core/window.hpp"
#include "debug_marker.hpp"
#include "r_globals.hpp"
#include "r_init.hpp"
#include "r_texture_manager.hpp"
#include "renderer/debug_ui.hpp"
#include "renderer/graphics_api/pg_to_vulkan_types.hpp"
#include "shared/logger.hpp"
#include "taskgraph/r_taskGraph.hpp"

using namespace PG;
using namespace Gfx;

namespace PG::RenderSystem
{

static VkPipeline s_gradientPipeline;
static VkPipelineLayout s_gradientPipelineLayout;
static VkPipeline s_meshPipeline;
static VkPipelineLayout s_meshPipelineLayout;
static TaskGraph s_taskGraph;

void ComputeDrawFunc( ComputeTask* task, TGExecuteData* data )
{
    CommandBuffer& cmdBuf = *data->cmdBuf;

#if USING( PG_DESCRIPTOR_BUFFER )
    VkDescriptorBufferBindingInfoEXT pBindingInfos{};
    pBindingInfos.sType   = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT;
    pBindingInfos.address = TextureManager::GetBindlessDescriptorBuffer().buffer.GetDeviceAddress();
    pBindingInfos.usage   = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;
    vkCmdBindDescriptorBuffersEXT( cmdBuf, 1, &pBindingInfos );

    uint32_t bufferIndex      = 0; // index into pBindingInfos for vkCmdBindDescriptorBuffersEXT?
    VkDeviceSize bufferOffset = 0;
    vkCmdSetDescriptorBufferOffsetsEXT(
        cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, s_gradientPipelineLayout, 0, 1, &bufferIndex, &bufferOffset );
#else  // #if USING( PG_DESCRIPTOR_BUFFER )
    vkCmdBindDescriptorSets(
        cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, s_gradientPipelineLayout, 0, 1, &TextureManager::GetBindlessSet(), 0, nullptr );
#endif // #else // #if USING( PG_DESCRIPTOR_BUFFER )

    vkCmdBindPipeline( cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, s_gradientPipeline );

    struct ComputePushConstants
    {
        vec4 topColor;
        vec4 botColor;
        uint32_t imageIndex;
    };
    Texture* tex = data->taskGraph->GetTexture( task->outputTextures[0] );
    ComputePushConstants push{ vec4( 1, 0, 0, 1 ), vec4( 0, 0, 1, 1 ), tex->GetBindlessArrayIndex() };
    vkCmdPushConstants( cmdBuf, s_gradientPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof( ComputePushConstants ), &push );

    Shader* shader = AssetManager::Get<Shader>( "gradient" );
    vkCmdDispatch( cmdBuf, (uint32_t)std::ceil( tex->GetWidth() / (float)shader->reflectionData.workgroupSize.x ),
        (uint32_t)std::ceil( tex->GetHeight() / (float)shader->reflectionData.workgroupSize.y ), 1 );
}

void MeshDrawFunc( GraphicsTask* task, TGExecuteData* data )
{
    CommandBuffer& cmdBuf = *data->cmdBuf;

    vkCmdBindDescriptorSets(
        cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, s_meshPipelineLayout, 0, 1, &TextureManager::GetBindlessSet(), 0, nullptr );

    vkCmdBindPipeline( cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, s_meshPipeline );
    cmdBuf.SetViewport( SceneSizedViewport() );
    cmdBuf.SetScissor( SceneSizedScissor() );

    vkCmdDrawMeshTasksEXT( cmdBuf, 1, 1, 1 );
}

bool Init_TaskGraph()
{
    TaskGraphBuilder builder;
    ComputeTaskBuilder* cTask = builder.AddComputeTask( "gradient" );
    TGBTextureRef gradientImg =
        cTask->AddTextureOutput( "gradientImg", PixelFormat::R16_G16_B16_A16_FLOAT, vec4( 0, 1, 0, 1 ), SIZE_SCENE(), SIZE_SCENE() );
    cTask->SetFunction( ComputeDrawFunc );

    GraphicsTaskBuilder* mTask = builder.AddGraphicsTask( "mesh" );
    mTask->AddColorAttachment( gradientImg );
    mTask->SetFunction( MeshDrawFunc );

    TGBTextureRef swapImg =
        builder.RegisterExternalTexture( "swapchainImg", rg.swapchain.GetFormat(), SIZE_DISPLAY(), SIZE_DISPLAY(), 1, 1, 1,
            []( VkImage& img, VkImageView& view )
            {
                img  = rg.swapchain.GetImage();
                view = rg.swapchain.GetImageView();
            } );

    TransferTaskBuilder* tTask = builder.AddTransferTask( "copyToSwapchain" );
    tTask->BlitTexture( swapImg, gradientImg );

    GraphicsTaskBuilder* gTask = builder.AddGraphicsTask( "UI_2D" );
    gTask->AddColorAttachment( swapImg );
    gTask->SetFunction( []( GraphicsTask* task, TGExecuteData* data ) { UIOverlay::Render( *data->cmdBuf ); } );

    PresentTaskBuilder* pTask = builder.AddPresentTask();
    pTask->SetPresentationImage( swapImg );

    TaskGraph::CompileInfo compileInfo;
    compileInfo.sceneWidth    = rg.sceneWidth;
    compileInfo.sceneHeight   = rg.sceneHeight;
    compileInfo.displayWidth  = rg.displayWidth;
    compileInfo.displayHeight = rg.displayHeight;
    compileInfo.showStats     = false;
    if ( !s_taskGraph.Compile( builder, compileInfo ) )
    {
        LOG_ERR( "Could not compile the task graph" );
        return false;
    }
    TG_DEBUG_ONLY( s_taskGraph.Print() );

    return true;
}

void Init_GraphicsPipeline()
{
    Shader* shaders[2] = {
        AssetManager::Get<Shader>( "triMS" ),
        AssetManager::Get<Shader>( "triPS" ),
    };
    VkPipelineShaderStageCreateInfo shaderStages[2];
    uint32_t numShaderStages = 2;
    for ( size_t i = 0; i < 2; ++i )
    {
        shaderStages[i]        = {};
        shaderStages[i].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[i].stage  = PGToVulkanShaderStage( shaders[i]->shaderStage );
        shaderStages[i].module = shaders[i]->handle;
        shaderStages[i].pName  = "main";
    }

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount             = 1;
    pipelineLayoutCreateInfo.pSetLayouts                = &TextureManager::GetBindlessSetLayout();
    pipelineLayoutCreateInfo.pushConstantRangeCount     = 0;
    VK_CHECK( vkCreatePipelineLayout( rg.device, &pipelineLayoutCreateInfo, NULL, &s_meshPipelineLayout ) );

    uint32_t dynamicStateCount       = 2;
    VkDynamicState vkDnamicStates[3] = { VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_VIEWPORT };

    VkPipelineDynamicStateCreateInfo dynamicState = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    dynamicState.dynamicStateCount                = dynamicStateCount;
    dynamicState.pDynamicStates                   = vkDnamicStates;

    VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    viewportState.viewportCount                     = 1;
    viewportState.scissorCount                      = 1;

    // VkPipelineInputAssemblyStateCreateInfo inputAssembly = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    // inputAssembly.topology                               = PGToVulkanPrimitiveType( PrimitiveType::TRIANGLES );
    // inputAssembly.primitiveRestartEnable                 = VK_FALSE;

    VkPipelineRasterizationStateCreateInfo rasterizer = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    rasterizer.depthClampEnable                       = VK_FALSE;
    rasterizer.rasterizerDiscardEnable                = VK_FALSE;
    rasterizer.polygonMode                            = PGToVulkanPolygonMode( PolygonMode::FILL );
    rasterizer.lineWidth                              = 1.0f; // to be higher than 1 needs the wideLines GPU feature
    rasterizer.cullMode                               = PGToVulkanCullFace( CullFace::BACK );
    rasterizer.frontFace                              = PGToVulkanWindingOrder( WindingOrder::COUNTER_CLOCKWISE );
    rasterizer.depthBiasEnable                        = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    multisampling.sampleShadingEnable                  = VK_FALSE;
    multisampling.rasterizationSamples                 = VK_SAMPLE_COUNT_1_BIT;

    uint8_t numColorAttachments                                 = 1;
    VkPipelineColorBlendAttachmentState colorBlendAttachment[8] = {};
    for ( uint8_t i = 0; i < numColorAttachments; ++i )
    {
        colorBlendAttachment[i].colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment[i].blendEnable = false;
    }

    // blending for all attachments / global settings
    VkPipelineColorBlendStateCreateInfo colorBlending = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    colorBlending.logicOpEnable                       = VK_FALSE;
    colorBlending.logicOp                             = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount                     = numColorAttachments;
    colorBlending.pAttachments                        = colorBlendAttachment;

    VkPipelineDepthStencilStateCreateInfo depthStencil = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
    depthStencil.depthTestEnable                       = false;
    depthStencil.depthWriteEnable                      = false;
    depthStencil.depthCompareOp                        = PGToVulkanCompareFunction( CompareFunction::LEQUAL );
    depthStencil.depthBoundsTestEnable                 = VK_FALSE;
    depthStencil.stencilTestEnable                     = VK_FALSE;

    VkGraphicsPipelineCreateInfo pipelineInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
    pipelineInfo.stageCount                   = numShaderStages;
    pipelineInfo.pStages                      = shaderStages;
    pipelineInfo.pVertexInputState            = nullptr;
    pipelineInfo.pInputAssemblyState          = nullptr;
    pipelineInfo.pViewportState               = &viewportState;
    pipelineInfo.pRasterizationState          = &rasterizer;
    pipelineInfo.pMultisampleState            = &multisampling;
    pipelineInfo.pDepthStencilState           = &depthStencil;
    pipelineInfo.pColorBlendState             = &colorBlending;
    pipelineInfo.pDynamicState                = &dynamicState;
    pipelineInfo.layout                       = s_meshPipelineLayout;

    VkFormat colorFmt                                       = PGToVulkanPixelFormat( PixelFormat::R16_G16_B16_A16_FLOAT );
    VkPipelineRenderingCreateInfoKHR dynRenderingCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR };
    dynRenderingCreateInfo.colorAttachmentCount             = 1;
    dynRenderingCreateInfo.pColorAttachmentFormats          = &colorFmt;

    // PixelFormat depthFmt = PixelFormat::INVALID;
    // if ( desc.dynamicAttachmentInfo.depthAttachmentFormat != PixelFormat::INVALID )
    //{
    //     VkFormat fmt                                 = PGToVulkanPixelFormat( depthFmt );
    //     dynRenderingCreateInfo.depthAttachmentFormat = fmt;
    //     if ( PixelFormatHasStencil( depthFmt ) )
    //         dynRenderingCreateInfo.stencilAttachmentFormat = fmt;
    // }

    pipelineInfo.pNext = &dynRenderingCreateInfo;

    if ( vkCreateGraphicsPipelines( rg.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &s_meshPipeline ) != VK_SUCCESS )
    {
        LOG_ERR( "Failed to create graphics pipeline" );
        vkDestroyPipelineLayout( rg.device, s_meshPipelineLayout, nullptr );
        return;
    }

    PG_DEBUG_MARKER_SET_PIPELINE_LAYOUT_NAME( s_meshPipelineLayout, "mesh" );
    PG_DEBUG_MARKER_SET_PIPELINE_NAME( s_meshPipeline, "mesh" );
}

bool Init( uint32_t sceneWidth, uint32_t sceneHeight, uint32_t displayWidth, uint32_t displayHeight, bool headless )
{
    rg.sceneWidth    = sceneWidth;
    rg.sceneHeight   = sceneHeight;
    rg.displayWidth  = displayWidth;
    rg.displayHeight = displayHeight;

    if ( !R_Init( headless, displayWidth, displayHeight ) )
        return false;

    if ( !AssetManager::LoadFastFile( "gfx_required" ) )
        return false;

    Shader* shader = AssetManager::Get<Shader>( "gradient" );

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    pipelineLayoutCreateInfo.setLayoutCount         = 1;
    pipelineLayoutCreateInfo.pSetLayouts            = &TextureManager::GetBindlessSetLayout();
    VkPushConstantRange pRange                      = { VK_SHADER_STAGE_COMPUTE_BIT, 0, shader->reflectionData.pushConstantSize };
    pipelineLayoutCreateInfo.pushConstantRangeCount = pRange.size ? 1 : 0;
    pipelineLayoutCreateInfo.pPushConstantRanges    = pRange.size ? &pRange : nullptr;
    VK_CHECK( vkCreatePipelineLayout( rg.device, &pipelineLayoutCreateInfo, NULL, &s_gradientPipelineLayout ) );

    VkPipelineShaderStageCreateInfo stageinfo{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
    stageinfo.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
    stageinfo.module = shader->handle;
    stageinfo.pName  = "main";

    VkComputePipelineCreateInfo computePipelineCreateInfo{ VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
#if USING( PG_DESCRIPTOR_BUFFER )
    computePipelineCreateInfo.flags |= VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
#endif // #if USING( PG_DESCRIPTOR_BUFFER )
    computePipelineCreateInfo.layout = s_gradientPipelineLayout;
    computePipelineCreateInfo.stage  = stageinfo;
    VK_CHECK( vkCreateComputePipelines( rg.device, nullptr, 1, &computePipelineCreateInfo, nullptr, &s_gradientPipeline ) );
    PG_DEBUG_MARKER_SET_PIPELINE_LAYOUT_NAME( s_gradientPipelineLayout, "gradient" );
    PG_DEBUG_MARKER_SET_PIPELINE_NAME( s_gradientPipeline, "gradient" );

    Init_GraphicsPipeline();

    if ( !UIOverlay::Init( rg.swapchain.GetFormat() ) )
        return false;

    if ( !Init_TaskGraph() )
        return false;

    return true;
}

void Resize( uint32_t displayWidth, uint32_t displayHeight )
{
    float oldRatioX = rg.sceneWidth / (float)rg.displayWidth;
    float oldRatioY = rg.sceneHeight / (float)rg.displayHeight;
    rg.sceneWidth   = static_cast<uint32_t>( displayWidth * oldRatioX + 0.5f );
    rg.sceneHeight  = static_cast<uint32_t>( displayHeight * oldRatioY + 0.5f );
    rg.swapchain.Recreate( displayWidth, displayHeight );
    LOG( "Resizing swapchain. Old size (%u x %u), new (%u x %u)", rg.displayWidth, rg.displayHeight, rg.swapchain.GetWidth(),
        rg.swapchain.GetHeight() );
    rg.displayWidth  = rg.swapchain.GetWidth();
    rg.displayHeight = rg.swapchain.GetHeight();
    s_taskGraph.Free();
    Init_TaskGraph();

    eg.resizeRequested = false;
}

void Shutdown()
{
    rg.device.WaitForIdle();
    s_taskGraph.Free();
    UIOverlay::Shutdown();
    vkDestroyPipelineLayout( rg.device, s_meshPipelineLayout, nullptr );
    vkDestroyPipeline( rg.device, s_meshPipeline, nullptr );
    vkDestroyPipelineLayout( rg.device, s_gradientPipelineLayout, nullptr );
    vkDestroyPipeline( rg.device, s_gradientPipeline, nullptr );

    AssetManager::FreeRemainingGpuResources();
    R_Shutdown();
}

void Render()
{
    FrameData& frameData = rg.GetFrameData();
    frameData.renderingCompleteFence.WaitFor();
    frameData.renderingCompleteFence.Reset();
    if ( !rg.swapchain.AcquireNextImage( frameData.swapchainSemaphore ) )
    {
        eg.resizeRequested = true;
        return;
    }

    TextureManager::Update();
    UIOverlay::BeginFrame();

    CommandBuffer& cmdBuf = frameData.primaryCmdBuffer;
    cmdBuf.Reset();
    cmdBuf.BeginRecording( COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT );

    TGExecuteData tgData;
    tgData.scene     = GetPrimaryScene();
    tgData.frameData = &frameData;
    tgData.cmdBuf    = &cmdBuf;
    s_taskGraph.Execute( tgData );

    UIOverlay::EndFrame();

    cmdBuf.EndRecording();

    VkPipelineStageFlags2 waitStages =
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    VkSemaphoreSubmitInfo waitInfo   = SemaphoreSubmitInfo( waitStages, frameData.swapchainSemaphore );
    VkSemaphoreSubmitInfo signalInfo = SemaphoreSubmitInfo( VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, frameData.renderingCompleteSemaphore );
    rg.device.Submit( cmdBuf, &waitInfo, &signalInfo, &frameData.renderingCompleteFence );

    if ( !rg.device.Present( rg.swapchain, frameData.renderingCompleteSemaphore ) )
    {
        eg.resizeRequested = true;
    }

    ++rg.currentFrameIdx;
}

void CreateTLAS( Scene* scene ) {}
::PG::Gfx::PipelineAttachmentInfo GetPipelineAttachmentInfo( const std::string& name ) { return {}; }

} // namespace PG::RenderSystem
