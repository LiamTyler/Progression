#include "render_system.hpp"
#include "asset/asset_manager.hpp"
#include "core/engine_globals.hpp"
#include "core/scene.hpp"
#include "core/window.hpp"
#include "debug_marker.hpp"
#include "ecs/components/model_renderer.hpp"
#include "ecs/components/transform.hpp"
#include "r_dvars.hpp"
#include "r_globals.hpp"
#include "r_init.hpp"
#include "r_texture_manager.hpp"
#include "renderer/debug_ui.hpp"
#include "renderer/graphics_api/pg_to_vulkan_types.hpp"
#include "shared/logger.hpp"
#include "taskgraph/r_taskGraph.hpp"

using namespace PG;
using namespace Gfx;

#define MAX_OBJECTS_PER_FRAME 4096

namespace PG::RenderSystem
{

static Pipeline s_computePipeline;
static Pipeline s_meshPipeline;
static TaskGraph s_taskGraph;
static Buffer s_buffer;

static VkQueryPool s_timestampQueryPool = VK_NULL_HANDLE;
static std::vector<uint64_t> s_timestamps;

void ComputeDrawFunc( ComputeTask* task, TGExecuteData* data )
{
    CommandBuffer& cmdBuf = *data->cmdBuf;

    cmdBuf.BindPipeline( &s_computePipeline );
    cmdBuf.BindGlobalDescriptors();

    struct ComputePushConstants
    {
        vec4 topColor;
        vec4 botColor;
        uint32_t imageIndex;
    };
    Texture* tex = data->taskGraph->GetTexture( task->outputTextures[0] );
    ComputePushConstants push{ vec4( 1, 0, 0, 1 ), vec4( 0, 0, 1, 1 ), tex->GetBindlessArrayIndex() };
    cmdBuf.PushConstants( push );
    cmdBuf.Dispatch_AutoSized( tex->GetWidth(), tex->GetHeight(), 1 );
}

void MeshDrawFunc( GraphicsTask* task, TGExecuteData* data )
{
    CommandBuffer& cmdBuf = *data->cmdBuf;
    cmdBuf.BindPipeline( &s_meshPipeline );
    cmdBuf.BindGlobalDescriptors();

    VkDeviceAddress address = s_buffer.GetDeviceAddress();
    cmdBuf.PushConstants( address );

    cmdBuf.SetViewport( SceneSizedViewport() );
    cmdBuf.SetScissor( SceneSizedScissor() );

    cmdBuf.DrawMeshTasks( 1, 1, 1 );
}

void UI_2D_DrawFunc( GraphicsTask* task, TGExecuteData* data )
{
    double timestampDiffToMillis = rg.physicalDevice.GetProperties().nanosecondsPerTick / 1e6;

    static size_t frameCount = 0;
    static double diff       = 0;
    // if ( !(frameCount % 200) )
    {
        diff = ( s_timestamps[1] - s_timestamps[0] ) * timestampDiffToMillis;
    }
    ++frameCount;

    CommandBuffer& cmdBuf = *data->cmdBuf;

    // UIOverlay::AddDrawFunction( Profile::DrawResultsOnScreen );
    ImGui::SetNextWindowPos( { 5, 5 }, ImGuiCond_FirstUseEver );
    ImGui::Begin( "Profiling Stats" );

    if ( ImGui::BeginTable( "RenderPass Times", 2, ImGuiTableFlags_Borders ) )
    {
        ImGui::TableSetupColumn( "Render Pass" );
        ImGui::TableSetupColumn( "Time (ms)" );
        ImGui::TableHeadersRow();

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex( 0 );
        ImGui::Text( "Frame Total" );
        ImGui::TableSetColumnIndex( 1 );
        ImGui::Text( "%.3f", diff );
        ImGui::EndTable();
    }
    ImGui::End();
    UIOverlay::Render( cmdBuf );
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
    mTask->AddDepthAttachment( "sceneDepth", PixelFormat::DEPTH_32_FLOAT, SIZE_SCENE(), SIZE_SCENE(), 1.0f );
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
    gTask->SetFunction( UI_2D_DrawFunc );

    PresentTaskBuilder* pTask = builder.AddPresentTask();
    pTask->SetPresentationImage( swapImg );

    TaskGraph::CompileInfo compileInfo;
    compileInfo.sceneWidth    = rg.sceneWidth;
    compileInfo.sceneHeight   = rg.sceneHeight;
    compileInfo.displayWidth  = rg.displayWidth;
    compileInfo.displayHeight = rg.displayHeight;
    // compileInfo.mergeResources = false;
    // TG_DEBUG_ONLY( compileInfo.showStats = true );
    if ( !s_taskGraph.Compile( builder, compileInfo ) )
    {
        LOG_ERR( "Could not compile the task graph" );
        return false;
    }
    // TG_DEBUG_ONLY( s_taskGraph.Print() );

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

    // Profile::Init();

    if ( !AssetManager::LoadFastFile( "gfx_required" ) )
        return false;

    s_computePipeline = rg.device.NewComputePipeline( AssetManager::Get<Shader>( "gradient" ), "gradient2" );

    GraphicsPipelineCreateInfo info = {};
    info.shaders.push_back( AssetManager::Get<Shader>( "modelMS" ) );
    info.shaders.push_back( AssetManager::Get<Shader>( "litPS" ) );
    info.colorAttachments.emplace_back( PixelFormat::R16_G16_B16_A16_FLOAT );
    info.depthInfo.format = PixelFormat::DEPTH_32_FLOAT;
    // info.depthInfo.depthTestEnabled  = false;
    // info.depthInfo.depthWriteEnabled = false;

    s_meshPipeline = rg.device.NewGraphicsPipeline( info, "mesh" );

    if ( !UIOverlay::Init( rg.swapchain.GetFormat() ) )
        return false;

    if ( !Init_TaskGraph() )
        return false;

    std::vector<vec3> meshletPositions = {
        vec3( 0.5, -0.5, 0 ),
        vec3( 0.5, 0.5, 0 ),
        vec3( -0.5, 0.5, 0 ),
    };
    BufferCreateInfo bufInfo = {};
    bufInfo.size             = meshletPositions.size() * sizeof( vec3 );
    bufInfo.initalData       = meshletPositions.data();
    bufInfo.bufferUsage |= BufferUsage::STORAGE;
    bufInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    s_buffer      = rg.device.NewBuffer( bufInfo, "mesh" );

    s_timestamps.resize( 2 );
    VkQueryPoolCreateInfo query_pool_info{};
    query_pool_info.sType      = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    query_pool_info.queryType  = VK_QUERY_TYPE_TIMESTAMP;
    query_pool_info.queryCount = static_cast<uint32_t>( s_timestamps.size() );
    VK_CHECK( vkCreateQueryPool( rg.device, &query_pool_info, nullptr, &s_timestampQueryPool ) );

    for ( int i = 0; i < NUM_FRAME_OVERLAP; ++i )
    {
        FrameData& fData = rg.frameData[i];

        BufferCreateInfo oBufInfo       = {};
        oBufInfo.size                   = MAX_OBJECTS_PER_FRAME * sizeof( mat4 );
        oBufInfo.bufferUsage            = BufferUsage::STORAGE | BufferUsage::TRANSFER_DST | BufferUsage::DEVICE_ADDRESS;
        oBufInfo.flags                  = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        fData.objectModelMatricesBuffer = rg.device.NewBuffer( oBufInfo, "objectModelMatricesBuffer" );

        BufferCreateInfo sgBufInfo = {};
        sgBufInfo.size             = sizeof( GpuData::SceneGlobals );
        sgBufInfo.bufferUsage      = BufferUsage::UNIFORM | BufferUsage::TRANSFER_DST | BufferUsage::DEVICE_ADDRESS;
        sgBufInfo.flags            = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        fData.sceneGlobalsBuffer   = rg.device.NewBuffer( sgBufInfo, "sceneGlobalsBuffer" );
    }

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

    for ( int i = 0; i < NUM_FRAME_OVERLAP; ++i )
    {
        rg.frameData[i].objectModelMatricesBuffer.Free();
        rg.frameData[i].sceneGlobalsBuffer.Free();
    }

    s_taskGraph.Free();
    UIOverlay::Shutdown();
    s_computePipeline.Free();
    s_meshPipeline.Free();
    vkDestroyQueryPool( rg.device, s_timestampQueryPool, nullptr );
    s_buffer.Free();

    AssetManager::FreeRemainingGpuResources();
    // Profile::Shutdown();
    R_Shutdown();
}

static void UpdateGPUSceneData( Scene* scene )
{
    FrameData& frameData    = rg.GetFrameData();
    mat4* gpuObjectMatrices = reinterpret_cast<mat4*>( frameData.objectModelMatricesBuffer.GetMappedPtr() );

    auto view          = scene->registry.view<ModelRenderer, PG::Transform>();
    uint32_t objectNum = 0;
    for ( auto entity : view )
    {
        if ( objectNum == MAX_OBJECTS_PER_FRAME )
        {
            LOG_WARN( "Too many objects in the scene! Some objects may be missing when in final render" );
            break;
        }

        PG::Transform& transform     = view.get<PG::Transform>( entity );
        mat4 M                       = transform.Matrix();
        gpuObjectMatrices[objectNum] = M;
        // mat4 N                               = Transpose( Inverse( M ) );
        // gpuObjectMatrices[2 * objectNum + 1] = N;
        ++objectNum;
    }

    GpuData::SceneGlobals globalData;
    globalData.V                        = scene->camera.GetV();
    globalData.P                        = scene->camera.GetP();
    globalData.VP                       = scene->camera.GetVP();
    globalData.invVP                    = Inverse( scene->camera.GetVP() );
    globalData.cameraPos                = vec4( scene->camera.position, 1 );
    VEC3 skyTint                        = scene->skyTint * powf( 2.0f, scene->skyEVAdjust );
    globalData.cameraExposureAndSkyTint = vec4( powf( 2.0f, scene->camera.exposure ), skyTint );
    globalData.r_materialViz            = r_materialViz.GetUint();
    globalData.r_lightingViz            = r_lightingViz.GetUint();
    globalData.r_postProcessing         = r_postProcessing.GetBool();
    globalData.r_tonemap                = r_tonemap.GetUint();
    globalData.debugInt                 = r_debugInt.GetInt();
    globalData.debugUint                = r_debugUint.GetUint();
    globalData.debugFloat               = r_debugFloat.GetFloat();
    globalData.debug3                   = 0u;

    memcpy( frameData.sceneGlobalsBuffer.GetMappedPtr(), &globalData, sizeof( GpuData::SceneGlobals ) );

    VkDescriptorBufferInfo dBufferInfo = { frameData.sceneGlobalsBuffer.GetHandle(), 0, VK_WHOLE_SIZE };
    VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    write.dstSet          = GetGlobalDescriptorSet();
    write.dstBinding      = 2;
    write.descriptorCount = 1;
    write.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.pBufferInfo     = &dBufferInfo;
    vkUpdateDescriptorSets( rg.device, 1, &write, 0, nullptr );
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
    UpdateGPUSceneData( GetPrimaryScene() );

    CommandBuffer& cmdBuf = frameData.primaryCmdBuffer;
    cmdBuf.Reset();
    cmdBuf.BeginRecording( COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT );

    vkCmdResetQueryPool( cmdBuf, s_timestampQueryPool, 0, static_cast<uint32_t>( s_timestamps.size() ) );
    vkCmdWriteTimestamp( cmdBuf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, s_timestampQueryPool, 0 );
    // Profile::StartFrame( cmdBuf );
    // PG_PROFILE_GPU_START( cmdBuf, "Frame" );

    TextureManager::Update();
    UIOverlay::BeginFrame();

    TGExecuteData tgData;
    tgData.scene     = GetPrimaryScene();
    tgData.frameData = &frameData;
    tgData.cmdBuf    = &cmdBuf;
    s_taskGraph.Execute( tgData );

    UIOverlay::EndFrame();
    // PG_PROFILE_GPU_END( cmdBuf, "Frame" );
    vkCmdWriteTimestamp( cmdBuf, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, s_timestampQueryPool, 1 );

    cmdBuf.EndRecording();
    // Profile::EndFrame();

    VkPipelineStageFlags2 waitStages =
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    VkSemaphoreSubmitInfo waitInfo   = SemaphoreSubmitInfo( waitStages, frameData.swapchainSemaphore );
    VkSemaphoreSubmitInfo signalInfo = SemaphoreSubmitInfo( VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, frameData.renderingCompleteSemaphore );
    rg.device.Submit( cmdBuf, &waitInfo, &signalInfo, &frameData.renderingCompleteFence );

    if ( !rg.device.Present( rg.swapchain, frameData.renderingCompleteSemaphore ) )
    {
        eg.resizeRequested = true;
    }

    uint32_t count = static_cast<uint32_t>( s_timestamps.size() );

    // Fetch the time stamp results written in the command buffer submissions
    // A note on the flags used:
    //	VK_QUERY_RESULT_64_BIT: Results will have 64 bits. As time stamp values are on nano-seconds, this flag should always be used to
    // avoid 32 bit overflows
    //  VK_QUERY_RESULT_WAIT_BIT: Since we want to immediately display the results, we use this flag to have the CPU wait until the results
    //  are available
    vkGetQueryPoolResults( rg.device, s_timestampQueryPool, 0, count, s_timestamps.size() * sizeof( uint64_t ), s_timestamps.data(),
        sizeof( uint64_t ), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT );

    ++rg.currentFrameIdx;
}

void CreateTLAS( Scene* scene ) {}

} // namespace PG::RenderSystem
