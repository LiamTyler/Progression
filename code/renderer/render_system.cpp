#include "render_system.hpp"
#include "asset/asset_manager.hpp"
#include "c_shared/bindless.h"
#include "c_shared/cull_data.h"
#include "c_shared/dvar_defines.h"
#include "c_shared/scene_globals.h"
#include "core/bounding_box.hpp"
#include "core/cpu_profiling.hpp"
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
#include "renderer/debug_draw.hpp"
#include "renderer/debug_ui.hpp"
#include "renderer/graphics_api/pg_to_vulkan_types.hpp"
#include "shared/logger.hpp"
#include "taskgraph/r_taskGraph.hpp"

using namespace PG;
using namespace Gfx;

#define MAX_MODELS_PER_FRAME 4096u
#define MAX_MESHES_PER_FRAME 65536u

namespace PG::RenderSystem
{

static TaskGraph s_taskGraph;

void ComputeFrustumCullMeshes( ComputeTask* task, TGExecuteData* data )
{
    CommandBuffer& cmdBuf = *data->cmdBuf;

    GpuData::PerObjectData* meshDrawData = data->frameData->meshDrawDataBuffer.GetMappedPtr<GpuData::PerObjectData>();
    GpuData::CullData* cullData          = data->frameData->meshCullData.GetMappedPtr<GpuData::CullData>();
    u32 modelNum                         = 0;
    u32 meshNum                          = 0;

    data->scene->registry.view<ModelRenderer, Transform>().each(
        [&]( ModelRenderer& modelRenderer, PG::Transform& transform )
        {
            if ( modelNum == MAX_MODELS_PER_FRAME || meshNum == MAX_MESHES_PER_FRAME )
                return;

            const Model* model = modelRenderer.model;

            u32 meshesToAdd = Min( MAX_MESHES_PER_FRAME, (u32)model->meshes.size() );
            for ( u32 i = 0; i < meshesToAdd; ++i )
            {
                GpuData::PerObjectData constants;
                constants.bindlessRangeStart = model->meshes[i].bindlessBuffersSlot;
                constants.modelIdx           = modelNum;
                constants.materialIdx        = modelRenderer.materials[i]->GetBindlessIndex();
                meshDrawData[meshNum + i]    = constants;

                GpuData::CullData cData;
                cData.aabb.min        = model->meshAABBs[i].min;
                cData.aabb.max        = model->meshAABBs[i].max;
                cData.modelIndex      = modelNum;
                cData.numMeshlets     = model->meshes[i].numMeshlets;
                cullData[meshNum + i] = cData;

#if !USING( SHIP_BUILD )
                if ( r_visualizeBoundingBoxes.GetBool() )
                {
                    AABB transformedAABB   = model->meshAABBs[i].Transform( transform.Matrix() );
                    DebugDraw::Color color = DebugDraw::Color::GREEN;
                    if ( !rg.debugCullingFrustum.BoxInFrustum( transformedAABB ) )
                        color = DebugDraw::Color::RED;

                    DebugDraw::AddAABB( transformedAABB, color );
                }
#endif // #if !USING( SHIP_BUILD )
            }

            meshNum += meshesToAdd;
            ++modelNum;
        } );

    cmdBuf.BindPipeline( PipelineManager::GetPipeline( "frustum_cull_meshes" ) );
    cmdBuf.BindGlobalDescriptors();

    struct ComputePushConstants
    {
        VkDeviceAddress cullDataBuffer;
        VkDeviceAddress outputCountBuffer;
        VkDeviceAddress indirectCmdOutputBuffer;
        u32 numMeshes;
        u32 _pad;
    };
    ComputePushConstants push;
    push.cullDataBuffer          = data->frameData->meshCullData.GetDeviceAddress();
    push.outputCountBuffer       = data->taskGraph->GetBuffer( task->outputBuffers[1] )->GetDeviceAddress();
    push.indirectCmdOutputBuffer = data->taskGraph->GetBuffer( task->outputBuffers[0] )->GetDeviceAddress();
    push.numMeshes               = meshNum;
    cmdBuf.PushConstants( push );
    cmdBuf.Dispatch_AutoSized( meshNum, 1, 1 );
}

void ComputeDrawFunc( ComputeTask* task, TGExecuteData* data )
{
    CommandBuffer& cmdBuf = *data->cmdBuf;

    cmdBuf.BindPipeline( PipelineManager::GetPipeline( "compute_debug" ) );
    cmdBuf.BindGlobalDescriptors();

    struct ComputePushConstants
    {
        VkDeviceAddress cullBuffer;
        u32 imageIndex;
    };
    Texture* tex = data->taskGraph->GetTexture( task->outputTextures[0] );
    Buffer* buff = data->taskGraph->GetBuffer( task->inputBuffers[0] );
    ComputePushConstants push{ buff->GetDeviceAddress(), tex->GetBindlessIndex() };
    cmdBuf.PushConstants( push );
    cmdBuf.Dispatch( 2, 2, 1 );
}

void MeshDrawFunc( GraphicsTask* task, TGExecuteData* data )
{
    PGP_ZONE_SCOPEDN( "Mesh Pass" );
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

#if 1
    const Buffer& indirectMeshletDrawBuff  = *data->taskGraph->GetBuffer( task->inputBuffers[0] );
    const Buffer& indirectMeshletCountBuff = *data->taskGraph->GetBuffer( task->inputBuffers[1] );
    struct ComputePushConstants
    {
        VkDeviceAddress meshDrawBuffer;
        VkDeviceAddress meshletDrawBuffer;
    };
    ComputePushConstants push;
    push.meshDrawBuffer    = indirectMeshletDrawBuff.GetDeviceAddress();
    push.meshletDrawBuffer = indirectMeshletCountBuff.GetDeviceAddress();
    // cmdBuf.PushConstants( push );
    //
    // cmdBuf.DrawMeshTasksIndirectCount(
    //     indirectMeshletDrawBuff, 0, indirectMeshletCountBuff, 0, MAX_MESHES_PER_FRAME, sizeof( GpuData::MeshletDrawCommand ) );
#else
    u32 modelNum = 0;
    data->scene->registry.view<ModelRenderer, Transform>().each(
        [&]( ModelRenderer& modelRenderer, PG::Transform& transform )
        {
            if ( modelNum == MAX_MODELS_PER_FRAME )
                return;

            const Model* model = modelRenderer.model;

            for ( size_t i = 0; i < model->meshes.size(); ++i )
            {
                const Mesh& mesh = model->meshes[i];

                GpuData::PerObjectData constants;
                constants.bindlessRangeStart = mesh.bindlessBuffersSlot;
                constants.modelIdx           = modelNum;
                constants.materialIdx        = modelRenderer.materials[i]->GetBindlessIndex();

                cmdBuf.PushConstants( constants );

                PG_DEBUG_MARKER_INSERT_CMDBUF( cmdBuf, "Draw '%s' : '%s'", model->GetName(), mesh.name.c_str() );
                cmdBuf.DrawMeshTasks( mesh.numMeshlets, 1, 1 );
            }

            ++modelNum;
        } );
#endif
}

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
    Texture* inputTex  = data->taskGraph->GetTexture( task->inputTextures[0] );
    Texture* outputTex = data->taskGraph->GetTexture( task->outputTextures[0] );
    PG_ASSERT( inputTex->GetWidth() == outputTex->GetWidth() );
    PG_ASSERT( inputTex->GetHeight() == outputTex->GetHeight() );
    ComputePushConstants push{ inputTex->GetBindlessIndex(), outputTex->GetBindlessIndex() };
    cmdBuf.PushConstants( push );
    cmdBuf.Dispatch_AutoSized( inputTex->GetWidth(), inputTex->GetHeight(), 1 );
}

void UI_2D_DrawFunc( GraphicsTask* task, TGExecuteData* data )
{
    CommandBuffer& cmdBuf = *data->cmdBuf;

    UIOverlay::AddDrawFunction( Profile::DrawResultsOnScreen );
    UIOverlay::Render( cmdBuf );
}

void UI_3D_DrawFunc( GraphicsTask* task, TGExecuteData* data )
{
    PGP_ZONE_SCOPEDN( "UI 3D" );
    CommandBuffer& cmdBuf = *data->cmdBuf;

    // DebugDraw::AddLine( vec3( 0 ), vec3( 5, 0, 0 ) );
    // DebugDraw::AddAABB( { vec3( -2 ), vec3( 2 ) } );
#if !USING( SHIP_BUILD )
    if ( r_frustumCullingDebug.GetBool() )
    {
        DebugDraw::AddFrustum( rg.debugCullingFrustum, DebugDraw::Color::YELLOW );
    }
#endif // #if !USING( SHIP_BUILD )
    DebugDraw::Draw( cmdBuf );
}

bool Init_TaskGraph()
{
    PGP_ZONE_SCOPEDN( "Init_TaskGraph" );
    TaskGraphBuilder builder;
    ComputeTaskBuilder* cTask            = builder.AddComputeTask( "FrustumCullMeshes" );
    TGBBufferRef indirectMeshletDrawBuff = cTask->AddBufferOutput(
        "indirectMeshletDrawBuff", VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, sizeof( GpuData::MeshletDrawCommand ) * MAX_MESHES_PER_FRAME );
    TGBBufferRef indirectMeshletCountBuff =
        cTask->AddBufferOutput( "indirectMeshletCountBuff", VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, sizeof( u32 ), 0 );
    cTask->SetFunction( ComputeFrustumCullMeshes );

    GraphicsTaskBuilder* mTask = builder.AddGraphicsTask( "DrawMeshes" );
    mTask->AddBufferInput( indirectMeshletDrawBuff, BufferUsage::INDIRECT );
    mTask->AddBufferInput( indirectMeshletCountBuff, BufferUsage::INDIRECT );
    TGBTextureRef litOutput =
        mTask->AddColorAttachment( "litOutput", PixelFormat::R16_G16_B16_A16_FLOAT, vec4( 0, 0, 0, 1 ), SIZE_SCENE(), SIZE_SCENE() );
    TGBTextureRef sceneDepth = mTask->AddDepthAttachment( "sceneDepth", PixelFormat::DEPTH_32_FLOAT, SIZE_SCENE(), SIZE_SCENE(), 1.0f );
    mTask->SetFunction( MeshDrawFunc );

    cTask = builder.AddComputeTask( "ComputeDraw" );
    cTask->AddBufferInput( indirectMeshletCountBuff );
    cTask->AddTextureInput( litOutput );
    cTask->AddTextureOutput( litOutput );
    cTask->SetFunction( ComputeDrawFunc );

    TGBTextureRef swapImg = builder.RegisterExternalTexture(
        "swapchainImg", rg.swapchain.GetFormat(), SIZE_DISPLAY(), SIZE_DISPLAY(), 1, 1, 1, []() { return rg.swapchain.GetTexture(); } );

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

    GraphicsTaskBuilder* gTask = builder.AddGraphicsTask( "UI_2D" );
    gTask->AddColorAttachment( swapImg );
    gTask->SetFunction( UI_2D_DrawFunc );

    gTask = builder.AddGraphicsTask( "UI_3D" );
    gTask->AddColorAttachment( swapImg );
    gTask->AddDepthAttachment( sceneDepth );
    gTask->SetFunction( UI_3D_DrawFunc );

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

    Profile::Init();

    if ( !Init_TaskGraph() )
        return false;

    if ( !AssetManager::LoadFastFile( "gfx_required" ) )
        return false;

    if ( !UIOverlay::Init( rg.swapchain.GetFormat() ) )
        return false;

    for ( i32 i = 0; i < NUM_FRAME_OVERLAP; ++i )
    {
        FrameData& fData = rg.frameData[i];

        BufferCreateInfo modelBufInfo = {};
        modelBufInfo.size             = MAX_MODELS_PER_FRAME * sizeof( mat4 );
        modelBufInfo.bufferUsage      = BufferUsage::STORAGE | BufferUsage::TRANSFER_DST | BufferUsage::DEVICE_ADDRESS;
        modelBufInfo.flags            = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        fData.modelMatricesBuffer     = rg.device.NewBuffer( modelBufInfo, "modelMatricesBuffer" );
        fData.normalMatricesBuffer    = rg.device.NewBuffer( modelBufInfo, "normalMatricesBuffer" );

        modelBufInfo.size        = MAX_MESHES_PER_FRAME * sizeof( GpuData::CullData );
        fData.meshCullData       = rg.device.NewBuffer( modelBufInfo, "meshCullData" );
        modelBufInfo.size        = MAX_MESHES_PER_FRAME * sizeof( GpuData::PerObjectData );
        fData.meshDrawDataBuffer = rg.device.NewBuffer( modelBufInfo, "meshDrawDataBuffer" );

        BufferCreateInfo sgBufInfo = {};
        sgBufInfo.size             = sizeof( GpuData::SceneGlobals );
        sgBufInfo.bufferUsage      = BufferUsage::UNIFORM | BufferUsage::TRANSFER_DST | BufferUsage::DEVICE_ADDRESS;
        sgBufInfo.flags            = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        fData.sceneGlobalsBuffer   = rg.device.NewBuffer( sgBufInfo, "sceneGlobalsBuffer" );
    }

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
    for ( i32 i = 0; i < NUM_FRAME_OVERLAP; ++i )
    {
        rg.frameData[i].meshCullData.Free();
        rg.frameData[i].meshDrawDataBuffer.Free();
        rg.frameData[i].modelMatricesBuffer.Free();
        rg.frameData[i].normalMatricesBuffer.Free();
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
    PGP_ZONE_SCOPEDN( "UpdateGPUSceneData" );
    FrameData& frameData     = rg.GetFrameData();
    mat4* gpuModelMatricies  = reinterpret_cast<mat4*>( frameData.modelMatricesBuffer.GetMappedPtr() );
    mat4* gpuNormalMatricies = reinterpret_cast<mat4*>( frameData.normalMatricesBuffer.GetMappedPtr() );

    auto view    = scene->registry.view<ModelRenderer, PG::Transform>();
    u32 modelNum = 0;
    for ( auto entity : view )
    {
        if ( modelNum == MAX_MODELS_PER_FRAME )
        {
            LOG_WARN( "Too many objects in the scene! Some objects may be missing when in final render" );
            break;
        }

        PG::Transform& transform    = view.get<PG::Transform>( entity );
        mat4 M                      = transform.Matrix();
        gpuModelMatricies[modelNum] = M;

        mat4 N                       = Transpose( Inverse( M ) );
        gpuNormalMatricies[modelNum] = N;
        ++modelNum;
    }

    GpuData::SceneGlobals globalData;
    memset( &globalData, 0, sizeof( globalData ) );
    globalData.V                        = scene->camera.GetV();
    globalData.P                        = scene->camera.GetP();
    globalData.VP                       = scene->camera.GetVP();
    globalData.invVP                    = Inverse( scene->camera.GetVP() );
    globalData.cameraPos                = vec4( scene->camera.position, 1 );
    vec3 skyTint                        = scene->skyTint * powf( 2.0f, scene->skyEVAdjust );
    globalData.cameraExposureAndSkyTint = vec4( powf( 2.0f, scene->camera.exposure ), skyTint );
    memcpy( globalData.frustumPlanes, scene->camera.GetFrustum().planes, sizeof( vec4 ) * 6 );
    globalData.modelMatriciesBufferIndex  = frameData.modelMatricesBuffer.GetBindlessIndex();
    globalData.normalMatriciesBufferIndex = frameData.normalMatricesBuffer.GetBindlessIndex();
    globalData.r_tonemap                  = r_tonemap.GetUint();

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

    if ( !r_frustumCullingDebug.GetBool() )
    {
        rg.debugCullingFrustum = scene->camera.GetFrustum();
    }
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
    PGP_ZONE_SCOPEDN( "Render" );
    frameData.renderingCompleteFence.Reset();
    UpdateGPUSceneData( GetPrimaryScene() );

    PGP_MANUAL_ZONEN( __high, "CommandBufReset" );
    CommandBuffer& cmdBuf = frameData.primaryCmdBuffer;
    cmdBuf.Reset();
    cmdBuf.BeginRecording( COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT );
    PGP_MANUAL_ZONE_END( __high );
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

    VkPipelineStageFlags2 waitStages =
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT;
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
