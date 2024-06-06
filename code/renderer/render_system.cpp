#include "render_system.hpp"
#include "asset/asset_manager.hpp"
#include "c_shared/bindless.h"
#include "c_shared/dvar_defines.h"
#include "c_shared/scene_globals.h"
#include "core/engine_globals.hpp"
#include "core/scene.hpp"
#include "core/window.hpp"
#include "debug_marker.hpp"
#include "ecs/components/model_renderer.hpp"
#include "ecs/components/transform.hpp"
#include "r_bindless_manager.hpp"
#include "r_dvars.hpp"
#include "r_globals.hpp"
#include "r_init.hpp"
#include "r_pipeline_manager.hpp"
#include "renderer/debug_ui.hpp"
#include "renderer/graphics_api/pg_to_vulkan_types.hpp"
#include "shared/logger.hpp"
#include "taskgraph/r_taskGraph.hpp"

using namespace PG;
using namespace Gfx;

#define MAX_OBJECTS_PER_FRAME 4096

namespace PG::RenderSystem
{

static TaskGraph s_taskGraph;

void ComputeDrawFunc( ComputeTask* task, TGExecuteData* data )
{
    CommandBuffer& cmdBuf = *data->cmdBuf;

    cmdBuf.BindPipeline( PipelineManager::GetPipeline( "gradient" ) );
    cmdBuf.BindGlobalDescriptors();

    struct ComputePushConstants
    {
        vec4 topColor;
        vec4 botColor;
        uint32_t imageIndex;
    };
    Texture* tex = data->taskGraph->GetTexture( task->outputTextures[0] );
    ComputePushConstants push{ vec4( 1, 0, 0, 1 ), vec4( 0, 0, 1, 1 ), tex->GetBindlessIndex() };
    cmdBuf.PushConstants( push );
    cmdBuf.Dispatch_AutoSized( tex->GetWidth(), tex->GetHeight(), 1 );
}

void MeshDrawFunc( GraphicsTask* task, TGExecuteData* data )
{
    CommandBuffer& cmdBuf = *data->cmdBuf;

    bool useDebugShader = false;
#if !USING( SHIP_BUILD )
    useDebugShader = useDebugShader || r_geometryViz.GetUint();
    useDebugShader = useDebugShader || r_materialViz.GetUint();
    useDebugShader = useDebugShader || r_lightingViz.GetUint();
    useDebugShader = useDebugShader || r_meshletViz.GetBool();
    useDebugShader = useDebugShader || r_wireframe.GetBool();
#endif // #if !USING( SHIP_BUILD )
    cmdBuf.BindPipeline( PipelineManager::GetPipeline( "litModel", useDebugShader ) );

    cmdBuf.BindGlobalDescriptors();

    cmdBuf.SetViewport( SceneSizedViewport() );
    cmdBuf.SetScissor( SceneSizedScissor() );

    uint32_t objectNum = 0;
    data->scene->registry.view<ModelRenderer, Transform>().each(
        [&]( ModelRenderer& modelRenderer, PG::Transform& transform )
        {
            if ( objectNum == MAX_OBJECTS_PER_FRAME )
                return;

            const Model* model = modelRenderer.model;

            for ( size_t i = 0; i < model->meshes.size(); ++i )
            {
                const Mesh& mesh = model->meshes[i];

                GpuData::PerObjectData constants;
                constants.bindlessRangeStart = mesh.bindlessBuffersSlot;
                constants.objectIdx          = objectNum;

                cmdBuf.PushConstants( constants );

                PG_DEBUG_MARKER_INSERT_CMDBUF( cmdBuf, "Draw '%s' : '%s'", model->GetName(), mesh.name.c_str() );
                cmdBuf.DrawMeshTasks( mesh.numMeshlets, 1, 1 );

                ++objectNum;
            }
        } );
}

void UI_2D_DrawFunc( GraphicsTask* task, TGExecuteData* data )
{
    CommandBuffer& cmdBuf = *data->cmdBuf;

    UIOverlay::AddDrawFunction( Profile::DrawResultsOnScreen );
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

    Profile::Init();

    if ( !Init_TaskGraph() )
        return false;

    if ( !AssetManager::LoadFastFile( "gfx_required" ) )
        return false;

    if ( !UIOverlay::Init( rg.swapchain.GetFormat() ) )
        return false;

    for ( int i = 0; i < NUM_FRAME_OVERLAP; ++i )
    {
        FrameData& fData = rg.frameData[i];

        BufferCreateInfo oBufInfo        = {};
        oBufInfo.size                    = MAX_OBJECTS_PER_FRAME * sizeof( mat4 );
        oBufInfo.bufferUsage             = BufferUsage::STORAGE | BufferUsage::TRANSFER_DST | BufferUsage::DEVICE_ADDRESS;
        oBufInfo.flags                   = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        fData.objectModelMatricesBuffer  = rg.device.NewBuffer( oBufInfo, "objectModelMatricesBuffer" );
        fData.objectNormalMatricesBuffer = rg.device.NewBuffer( oBufInfo, "objectNormalMatricesBuffer" );

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
        rg.frameData[i].objectNormalMatricesBuffer.Free();
        rg.frameData[i].sceneGlobalsBuffer.Free();
    }

    s_taskGraph.Free();
    UIOverlay::Shutdown();
    AssetManager::FreeRemainingGpuResources();
    Profile::Shutdown();
    R_Shutdown();
}

static void UpdateGPUSceneData( Scene* scene )
{
    FrameData& frameData     = rg.GetFrameData();
    mat4* gpuModelMatricies  = reinterpret_cast<mat4*>( frameData.objectModelMatricesBuffer.GetMappedPtr() );
    mat4* gpuNormalMatricies = reinterpret_cast<mat4*>( frameData.objectNormalMatricesBuffer.GetMappedPtr() );

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
        gpuModelMatricies[objectNum] = M;

        mat4 N                        = Transpose( Inverse( M ) );
        gpuNormalMatricies[objectNum] = N;
        ++objectNum;
    }

    GpuData::SceneGlobals globalData;
    memset( &globalData, 0, sizeof( globalData ) );
    globalData.V                        = scene->camera.GetV();
    globalData.P                        = scene->camera.GetP();
    globalData.VP                       = scene->camera.GetVP();
    globalData.invVP                    = Inverse( scene->camera.GetVP() );
    globalData.cameraPos                = vec4( scene->camera.position, 1 );
    VEC3 skyTint                        = scene->skyTint * powf( 2.0f, scene->skyEVAdjust );
    globalData.cameraExposureAndSkyTint = vec4( powf( 2.0f, scene->camera.exposure ), skyTint );
    globalData.modelMatriciesIdx        = frameData.objectModelMatricesBuffer.GetBindlessIndex();
    globalData.normalMatriciesIdx       = frameData.objectNormalMatricesBuffer.GetBindlessIndex();
    globalData.r_tonemap                = r_tonemap.GetUint();

#if !USING( SHIP_BUILD )
    globalData.r_geometryViz    = r_geometryViz.GetUint();
    globalData.r_materialViz    = r_materialViz.GetUint();
    globalData.r_lightingViz    = r_lightingViz.GetUint();
    globalData.r_postProcessing = r_postProcessing.GetBool();
    PackMeshletVizBit( globalData.debug_PackedDvarBools, r_meshletViz.GetBool() );
    PackWireframeBit( globalData.debug_PackedDvarBools, r_wireframe.GetBool() );
    vec4 wireframeColor              = r_wireframeColor.GetVec4();
    globalData.debug_wireframeData.r = wireframeColor.r;
    globalData.debug_wireframeData.g = wireframeColor.g;
    globalData.debug_wireframeData.b = wireframeColor.b;
    globalData.debug_wireframeData.a = r_wireframeThickness.GetFloat();
    globalData.debugInt              = r_debugInt.GetInt();
    globalData.debugUint             = r_debugUint.GetUint();
    globalData.debugFloat            = r_debugFloat.GetFloat();
#endif // #if !USING( SHIP_BUILD )

    memcpy( frameData.sceneGlobalsBuffer.GetMappedPtr(), &globalData, sizeof( GpuData::SceneGlobals ) );

    VkDescriptorBufferInfo dBufferInfo = { frameData.sceneGlobalsBuffer.GetHandle(), 0, VK_WHOLE_SIZE };
    VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    write.dstSet          = GetGlobalDescriptorSet();
    write.dstBinding      = PG_SCENE_GLOBALS_DSET_BINDING;
    write.descriptorCount = 1;
    write.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.pBufferInfo     = &dBufferInfo;
    vkUpdateDescriptorSets( rg.device, 1, &write, 0, nullptr );
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
    frameData.renderingCompleteFence.Reset();
    UpdateGPUSceneData( GetPrimaryScene() );

    CommandBuffer& cmdBuf = frameData.primaryCmdBuffer;
    cmdBuf.Reset();
    cmdBuf.BeginRecording( COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT );
    PG_DEBUG_MARKER_BEGIN_REGION_CMDBUF( cmdBuf, "Frame" );
    PG_PROFILE_GPU_START_FRAME( cmdBuf );
    PG_PROFILE_GPU_START( cmdBuf, Frame, "Frame" );

    BindlessManager::Update();
    UIOverlay::BeginFrame();

    TGExecuteData tgData;
    tgData.scene     = GetPrimaryScene();
    tgData.frameData = &frameData;
    tgData.cmdBuf    = &cmdBuf;
    s_taskGraph.Execute( tgData );

    PG_PROFILE_GPU_END( cmdBuf, Frame );
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

    rg.EndFrame();
}

void CreateTLAS( Scene* scene ) {}

} // namespace PG::RenderSystem
