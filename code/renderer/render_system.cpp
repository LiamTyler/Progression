#include "render_system.hpp"
#include "asset/asset_manager.hpp"
#include "core/init.hpp"
#include "core/window.hpp"
#include "r_globals.hpp"
#include "r_init.hpp"
#include "r_texture_manager.hpp"
#include "renderer/debug_ui.hpp"
#include "shared/logger.hpp"
#include "taskgraph/r_taskGraph.hpp"

using namespace PG;
using namespace Gfx;

namespace PG::RenderSystem
{

static VkPipeline s_gradientPipeline;
static VkPipelineLayout s_gradientPipelineLayout;
static TaskGraph s_taskGraph;

void NewDrawFunctionTG( ComputeTask* task, TGExecuteData* data )
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

bool Init_TaskGraph()
{
    TaskGraphBuilder builder;
    ComputeTaskBuilder* cTask = builder.AddComputeTask( "gradient" );
    TGBTextureRef gradientImg =
        cTask->AddTextureOutput( "gradientImg", PixelFormat::R16_G16_B16_A16_FLOAT, vec4( 0, 1, 0, 1 ), SIZE_SCENE(), SIZE_SCENE() );
    cTask->SetFunction( NewDrawFunctionTG );

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
    if ( !s_taskGraph.Compile( builder, compileInfo ) )
    {
        LOG_ERR( "Could not compile the task graph" );
        return false;
    }

    return true;
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

    VK_CHECK( vkCreateComputePipelines( rg.device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &s_gradientPipeline ) );

    if ( !UIOverlay::Init( rg.swapchain.GetFormat() ) )
        return false;

    if ( !Init_TaskGraph() )
        return false;

    return true;
}

void Resize( uint32_t displayWidth, uint32_t displayHeight )
{
    float oldRatioX  = rg.sceneWidth / (float)rg.displayWidth;
    float oldRatioY  = rg.sceneHeight / (float)rg.displayHeight;
    uint32_t oldDW   = rg.displayWidth;
    uint32_t oldDH   = rg.displayHeight;
    rg.displayWidth  = displayWidth;
    rg.displayHeight = displayHeight;
    rg.sceneWidth    = static_cast<uint32_t>( displayWidth * oldRatioX + 0.5f );
    rg.sceneHeight   = static_cast<uint32_t>( displayHeight * oldRatioY + 0.5f );
    rg.swapchain.Recreate( displayWidth, displayHeight );
    s_taskGraph.Free();
    Init_TaskGraph();
    LOG( "Old size (%u x %u), new (%u x %u)", oldDW, oldDH, rg.displayWidth, rg.displayHeight );

    g_resizeRequested = false;
}

void Shutdown()
{
    rg.device.WaitForIdle();
    s_taskGraph.Free();
    UIOverlay::Shutdown();
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
        g_resizeRequested = true;
        return;
    }

    TextureManager::Update();
    UIOverlay::BeginFrame();

    CommandBuffer& cmdBuf = frameData.primaryCmdBuffer;
    cmdBuf.Reset();
    cmdBuf.BeginRecording( COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT );

    TGExecuteData tgData;
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
        g_resizeRequested = true;
    }

    ++rg.currentFrameIdx;
}

void CreateTLAS( Scene* scene ) {}
::PG::Gfx::PipelineAttachmentInfo GetPipelineAttachmentInfo( const std::string& name ) { return {}; }

} // namespace PG::RenderSystem
