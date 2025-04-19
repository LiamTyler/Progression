#include "render_system.hpp"
#include "asset/asset_manager.hpp"
#include "core/cpu_profiling.hpp"
#include "core/engine_globals.hpp"
#include "core/window.hpp"
#include "debug_draw.hpp"
#include "debug_marker.hpp"
#include "debug_ui.hpp"
#include "graphics_api/gpu_profiling.hpp"
#include "r_bindless_manager.hpp"
#include "r_dvars.hpp"
#include "r_globals.hpp"
#include "r_init.hpp"
#include "r_lighting.hpp"
#include "r_pipeline_manager.hpp"
#include "r_scene.hpp"
#include "r_sky.hpp"
#include "shared/logger.hpp"
#include "taskgraph/r_taskGraph.hpp"
#include "ui/ui_system.hpp"

using namespace PG;
using namespace Gfx;

namespace PG::RenderSystem
{

static TaskGraph s_taskGraph;

void PostProcessFunc( ComputeTask* task, TGExecuteData* data )
{
    CommandBuffer& cmdBuf = *data->cmdBuf;

    cmdBuf.BindPipeline( PipelineManager::GetPipeline( "post_process" ) );
    cmdBuf.BindGlobalDescriptors();

    struct ComputePushConstants
    {
        u32 inputImageIndex;
        u32 outputImageIndex;
    };
    const Texture& inputTex  = task->GetInputTexture( 0 );
    const Texture& outputTex = task->GetOutputTexture( 0 );
    PG_ASSERT( inputTex.GetWidth() == outputTex.GetWidth() );
    PG_ASSERT( inputTex.GetHeight() == outputTex.GetHeight() );
    ComputePushConstants push{ inputTex.GetBindlessIndex(), outputTex.GetBindlessIndex() };
    cmdBuf.PushConstants( push );
    cmdBuf.Dispatch_AutoSized( inputTex.GetWidth(), inputTex.GetHeight(), 1 );
}

void UI_2D_DrawFunc( GraphicsTask* task, TGExecuteData* data )
{
    CommandBuffer& cmdBuf = *data->cmdBuf;

    // UI::Text::TextDrawInfo tInfo;
    // tInfo.pos = vec2( 0.9, 0 );
    // UI::Text::Draw2D( tInfo, "Frustum: 11" );
    // tInfo.pos = vec2( 0.9, 0.1 );
    // UI::Text::Draw2D( tInfo, "Frustum: 12" );

    // UI::Text::Render( cmdBuf );
    UI::Render( cmdBuf );
    UIOverlay::AddDrawFunction( Profile::DrawResultsOnScreen );
    UIOverlay::Render( cmdBuf );
    DebugDraw::Draw2D( cmdBuf );
}

void UI_3D_DrawFunc( GraphicsTask* task, TGExecuteData* data )
{
    PGP_ZONE_SCOPEDN( "UI 3D" );
    CommandBuffer& cmdBuf = *data->cmdBuf;

#if USING( DEVELOPMENT_BUILD )
    if ( r_frustumCullingDebug.GetBool() )
    {
        DebugDraw::AddFrustum( rg.debugCullingFrustum, DebugDraw::Color::YELLOW );
    }
#endif // #if USING( DEVELOPMENT_BUILD )
    DebugDraw::Draw3D( cmdBuf );
}

bool Init_TaskGraph()
{
    PGP_ZONE_SCOPEDN( "Init_TaskGraph" );
    TaskGraphBuilder builder;

    ComputeTaskBuilder* cTask  = nullptr;
    GraphicsTaskBuilder* gTask = nullptr;

    TGBTextureRef litOutput, sceneDepth;
    AddSceneRenderTasks( builder, litOutput, sceneDepth );
    Lighting::AddShadowTasks( builder );

    Sky::AddTask( builder, litOutput, sceneDepth );

    TGBTextureRef swapImg = builder.RegisterExternalTexture( "swapchainImg", rg.swapchain.GetFormat(), rg.swapchain.GetWidth(),
        rg.swapchain.GetHeight(), 1, 1, 1, []() { return rg.swapchain.GetTexture(); } );

    TGBTextureRef litImgDisplaySized = litOutput;
    if ( rg.sceneWidth != rg.displayWidth || rg.sceneHeight != rg.displayHeight )
    {
        TransferTaskBuilder* tTask = builder.AddTransferTask( "copyAndResizeToSwapchain" );
        tTask->BlitTexture( swapImg, litOutput );
        litImgDisplaySized = swapImg;
    }

    cTask = builder.AddComputeTask( "post_process" );
    cTask->AddTextureInput( litImgDisplaySized );
    cTask->AddTextureOutput( swapImg );
    cTask->SetFunction( PostProcessFunc );

    gTask = builder.AddGraphicsTask( "UI_3D" );
    gTask->AddColorAttachment( swapImg );
    gTask->AddDepthAttachment( sceneDepth );
    gTask->SetFunction( UI_3D_DrawFunc );

    gTask = builder.AddGraphicsTask( "UI_2D" );
    gTask->AddColorAttachment( swapImg );
    gTask->SetFunction( UI_2D_DrawFunc );

    PresentTaskBuilder* pTask = builder.AddPresentTask();
    pTask->SetPresentationImage( swapImg );

    TaskGraph::CompileInfo compileInfo;
    compileInfo.sceneWidth     = rg.sceneWidth;
    compileInfo.sceneHeight    = rg.sceneHeight;
    compileInfo.displayWidth   = rg.displayWidth;
    compileInfo.displayHeight  = rg.displayHeight;
    compileInfo.mergeResources = false;
    // TG_DEBUG_ONLY( compileInfo.showStats = true );
    if ( !s_taskGraph.Compile( builder, compileInfo ) )
    {
        LOG_ERR( "Could not compile the task graph" );
        return false;
    }
    // TG_DEBUG_ONLY( s_taskGraph.Print() );

    return true;
}

bool Init( u32 sceneWidth, u32 sceneHeight, u32 displayWidth, u32 displayHeight, bool headless )
{
    PGP_ZONE_SCOPEDN( "RenderSystem::Init" );
    rg.sceneWidth    = sceneWidth;
    rg.sceneHeight   = sceneHeight;
    rg.displayWidth  = displayWidth;
    rg.displayHeight = displayHeight;

    if ( !R_Init( headless, displayWidth, displayHeight ) )
        return false;

    if ( !AssetManager::LoadFastFile( "gfx_required" ) )
        return false;

    Profile::Init();
    Sky::Init();

    if ( !Init_TaskGraph() )
        return false;

    if ( !UIOverlay::Init( rg.swapchain.GetFormat() ) )
        return false;

    Init_SceneData();

    Lighting::Init();
    DebugDraw::Init();

    return true;
}

void Resize( u32 displayWidth, u32 displayHeight )
{
    PGP_ZONE_SCOPEDN( "Resize" );
    f32 oldRatioX  = rg.sceneWidth / (f32)rg.displayWidth;
    f32 oldRatioY  = rg.sceneHeight / (f32)rg.displayHeight;
    rg.sceneWidth  = static_cast<u32>( displayWidth * oldRatioX + 0.5f );
    rg.sceneHeight = static_cast<u32>( displayHeight * oldRatioY + 0.5f );
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

    DebugDraw::Shutdown();
    Lighting::Shutdown();
    Shutdown_SceneData();

    s_taskGraph.Free();
    UIOverlay::Shutdown();
    AssetManager::FreeRemainingGpuResources();
    Sky::Shutdown();
    Profile::Shutdown();
    R_Shutdown();
}

void Render()
{
    FrameData& frameData = rg.GetFrameData();
    frameData.renderingCompleteFence.WaitFor();
    if ( !rg.swapchain.AcquireNextImage( frameData.swapchainSemaphore ) )
    {
        eg.resizeRequested = true;
        frameData.swapchainSemaphore.Unsignal();
        return;
    }
    PGP_ZONE_SCOPEDN( "Render" );
    frameData.renderingCompleteFence.Reset();
    UpdateSceneData( GetPrimaryScene() );

    PGP_MANUAL_ZONEN( __cmdBufReset, "CommandBufReset" );
    CommandBuffer& cmdBuf = frameData.primaryCmdBuffer;
    cmdBuf.Reset();
    cmdBuf.BeginRecording( COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT );
    PGP_MANUAL_ZONE_END( __cmdBufReset );
    PG_DEBUG_MARKER_BEGIN_REGION_CMDBUF( cmdBuf, "Frame" );
    PG_PROFILE_GPU_START_FRAME( cmdBuf );
    PG_PROFILE_GPU_START( cmdBuf, Frame, "Frame" );

    DebugDraw::StartFrame();
    BindlessManager::Update();
    UIOverlay::BeginFrame();
    rg.device.AcquirePendingTransfers();

    TGExecuteData tgData;
    tgData.scene     = GetPrimaryScene();
    tgData.frameData = &frameData;
    tgData.cmdBuf    = &cmdBuf;
    s_taskGraph.Execute( tgData );

    PG_PROFILE_GPU_END( cmdBuf, Frame );
    UIOverlay::EndFrame();
    cmdBuf.EndRecording();

    VkPipelineStageFlags2 waitStages = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    // VkPipelineStageFlags2 waitStages =
    //     VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    VkSemaphoreSubmitInfo waitInfo   = SemaphoreSubmitInfo( waitStages, frameData.swapchainSemaphore );
    VkSemaphoreSubmitInfo signalInfo = SemaphoreSubmitInfo( VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, frameData.renderingCompleteSemaphore );
    rg.device.Submit( QueueType::GRAPHICS, cmdBuf, &waitInfo, &signalInfo, &frameData.renderingCompleteFence );

    if ( !rg.device.Present( rg.swapchain, frameData.renderingCompleteSemaphore ) )
    {
        eg.resizeRequested = true;
    }

    rg.EndFrame();
}

} // namespace PG::RenderSystem
