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
#include "debug_draw.hpp"
#include "debug_marker.hpp"
#include "debug_ui.hpp"
#include "ecs/components/model_renderer.hpp"
#include "ecs/components/transform.hpp"
#include "graphics_api/pg_to_vulkan_types.hpp"
#include "r_bindless_manager.hpp"
#include "r_dvars.hpp"
#include "r_globals.hpp"
#include "r_init.hpp"
#include "r_lighting.hpp"
#include "r_pipeline_manager.hpp"
#include "r_sky.hpp"
#include "shared/logger.hpp"
#include "taskgraph/r_taskGraph.hpp"
#include "ui/ui_text.hpp"

using namespace PG;
using namespace Gfx;

#define MAX_MODELS_PER_FRAME 65536u
#define MAX_MESHES_PER_FRAME 65536u

#define INDIRECT_MAIN_DRAW IN_USE    // must keep in sync with litModel's #define in gfx_require.paf
#define TASK_SHADER_MAIN_DRAW IN_USE // must keep in sync with taskShader in gfx_require.paf

namespace PG::RenderSystem
{

static TaskGraph s_taskGraph;

#if USING( INDIRECT_MAIN_DRAW )
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

            u32 meshesToAdd = Min( MAX_MESHES_PER_FRAME - meshNum, (u32)model->meshes.size() );
            for ( u32 i = 0; i < meshesToAdd; ++i )
            {
                if ( modelRenderer.materials[i]->type == MaterialType::DECAL )
                    continue;

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

#if USING( DEVELOPMENT_BUILD )
                if ( r_visualizeBoundingBoxes.GetBool() )
                {
                    AABB transformedAABB = model->meshAABBs[i].Transform( transform.Matrix() );
                    if ( rg.debugCullingFrustum.BoxInFrustum( transformedAABB ) )
                        DebugDraw::AddAABB( transformedAABB );
                }
#endif // #if USING( DEVELOPMENT_BUILD )
            }

            meshNum += meshesToAdd;
            ++modelNum;
        } );

    data->frameData->numMeshes = meshNum;

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
    push.outputCountBuffer       = task->GetOutputBuffer( 0 ).GetDeviceAddress();
    push.indirectCmdOutputBuffer = task->GetOutputBuffer( 1 ).GetDeviceAddress();
    push.numMeshes               = meshNum;
    cmdBuf.PushConstants( push );
    cmdBuf.Dispatch_AutoSized( meshNum, 1, 1 );
}

void ComputeFrustumCullMeshes_Debug( ComputeTask* task, TGExecuteData* data )
{
    CommandBuffer& cmdBuf = *data->cmdBuf;

    cmdBuf.BindPipeline( PipelineManager::GetPipeline( "frustum_cull_meshes_debug" ) );
    cmdBuf.BindGlobalDescriptors();

    struct ComputePushConstants
    {
        VkDeviceAddress countBuffer;
        VkDeviceAddress meshDrawDataBuffer;
    };
    ComputePushConstants push;
    push.countBuffer        = task->GetInputBuffer( 0 ).GetDeviceAddress();
    push.meshDrawDataBuffer = task->GetInputBuffer( 1 ).GetDeviceAddress();
    cmdBuf.PushConstants( push );
    cmdBuf.Dispatch( 1, 1, 1 );
}
#endif // #if USING( INDIRECT_MAIN_DRAW )

void MeshDrawFunc( GraphicsTask* task, TGExecuteData* data )
{
    PGP_ZONE_SCOPEDN( "Mesh Pass" );
    CommandBuffer& cmdBuf = *data->cmdBuf;

    bool useDebugShader = false;
#if USING( DEVELOPMENT_BUILD )
    useDebugShader = useDebugShader || r_geometryViz.GetUint();
    useDebugShader = useDebugShader || r_materialViz.GetUint();
    useDebugShader = useDebugShader || r_lightingViz.GetUint();
    useDebugShader = useDebugShader || r_meshletViz.GetBool();
    useDebugShader = useDebugShader || r_wireframe.GetBool();
#endif // #if USING( DEVELOPMENT_BUILD )
    cmdBuf.BindPipeline( PipelineManager::GetPipeline( "litModel", useDebugShader ) );
    cmdBuf.BindGlobalDescriptors();
    cmdBuf.SetViewport( SceneSizedViewport() );
    cmdBuf.SetScissor( SceneSizedScissor() );

#if USING( INDIRECT_MAIN_DRAW )
    const Buffer& indirectCountBuff       = task->GetInputBuffer( 0 );
    const Buffer& indirectMeshletDrawBuff = task->GetInputBuffer( 1 );
    struct ComputePushConstants
    {
        VkDeviceAddress meshDataBuffer;
        VkDeviceAddress indirectMeshletDrawBuffer;
    };
    ComputePushConstants push;
    push.meshDataBuffer            = data->frameData->meshDrawDataBuffer.GetDeviceAddress();
    push.indirectMeshletDrawBuffer = indirectMeshletDrawBuff.GetDeviceAddress();
    cmdBuf.PushConstants( push );

    cmdBuf.DrawMeshTasksIndirectCount(
        indirectMeshletDrawBuff, 0, indirectCountBuff, 0, MAX_MESHES_PER_FRAME, sizeof( GpuData::MeshletDrawCommand ) );
#else // #if USING( INDIRECT_MAIN_DRAW )
    u32 modelNum = 0;
    u32 meshNum  = 0;
    data->scene->registry.view<ModelRenderer, Transform>().each(
        [&]( ModelRenderer& modelRenderer, PG::Transform& transform )
        {
            if ( modelNum == MAX_MODELS_PER_FRAME || meshNum == MAX_MESHES_PER_FRAME )
                return;

            const Model* model = modelRenderer.model;

            u32 meshesToAdd = Min( MAX_MESHES_PER_FRAME - meshNum, (u32)model->meshes.size() );
            for ( size_t i = 0; i < meshesToAdd; ++i )
            {
                if ( modelRenderer.materials[i]->type == MaterialType::DECAL )
                    continue;

                const Mesh& mesh = model->meshes[i];

                GpuData::NonIndirectPerObjectData constants;
                constants.numMeshlets        = mesh.numMeshlets;
                constants.bindlessRangeStart = mesh.bindlessBuffersSlot;
                constants.modelIdx           = modelNum;
                constants.materialIdx        = modelRenderer.materials[i]->GetBindlessIndex();

                cmdBuf.PushConstants( constants );

                PG_DEBUG_MARKER_INSERT_CMDBUF( cmdBuf, "Draw '%s' : '%s'", model->GetName(), mesh.name.c_str() );
#if USING( TASK_SHADER_MAIN_DRAW )
                cmdBuf.DrawMeshTasks( ( mesh.numMeshlets + 31 ) / 32, 1, 1 );
#else  // #if USING( TASK_SHADER_MAIN_DRAW )
                cmdBuf.DrawMeshTasks( mesh.numMeshlets, 1, 1 );
#endif // #else // #if USING( TASK_SHADER_MAIN_DRAW )
            }
            meshNum += meshesToAdd;

            ++modelNum;
        } );
    data->frameData->numMeshes = meshNum;
#endif // #else // #if USING( INDIRECT_MAIN_DRAW )
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

    UI::Text::Render( cmdBuf );
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
#if USING( INDIRECT_MAIN_DRAW )
    cTask                          = builder.AddComputeTask( "FrustumCullMeshes" );
    TGBBufferRef indirectCountBuff = cTask->AddBufferOutput( "indirectCountBuff", VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, sizeof( u32 ), 0 );
    TGBBufferRef indirectMeshletDrawBuff = cTask->AddBufferOutput(
        "indirectMeshletDrawBuff", VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, sizeof( GpuData::MeshletDrawCommand ) * MAX_MESHES_PER_FRAME );
    cTask->SetFunction( ComputeFrustumCullMeshes );
#endif // #if USING( INDIRECT_MAIN_DRAW )

    gTask = builder.AddGraphicsTask( "DrawMeshes" );
#if USING( INDIRECT_MAIN_DRAW )
    gTask->AddBufferInput( indirectCountBuff, BufferUsage::INDIRECT );
    gTask->AddBufferInput( indirectMeshletDrawBuff, BufferUsage::INDIRECT );
#endif // #if USING( INDIRECT_MAIN_DRAW )
    TGBTextureRef litOutput  = gTask->AddColorAttachment( "litOutput", PixelFormat::R16_G16_B16_A16_FLOAT, SIZE_SCENE(), SIZE_SCENE() );
    TGBTextureRef sceneDepth = gTask->AddDepthAttachment( "sceneDepth", PixelFormat::DEPTH_32_FLOAT, SIZE_SCENE(), SIZE_SCENE(), 1.0f );
    gTask->SetFunction( MeshDrawFunc );

#if USING( DEVELOPMENT_BUILD ) && USING( INDIRECT_MAIN_DRAW )
    cTask = builder.AddComputeTask( "ComputeDraw" );
    cTask->AddBufferInput( indirectCountBuff );
    cTask->AddBufferInput( indirectMeshletDrawBuff );
    cTask->SetFunction( ComputeFrustumCullMeshes_Debug );
#endif // #if USING( DEVELOPMENT_BUILD ) && USING( INDIRECT_MAIN_DRAW )

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

    Lighting::Init();
    UI::Text::Init();
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
    UI::Text::Shutdown();
    Lighting::Shutdown();
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
    Sky::Shutdown();
    Profile::Shutdown();
    R_Shutdown();
}

static void UpdateGPUSceneData( Scene* scene )
{
    PGP_ZONE_SCOPEDN( "UpdateGPUSceneData" );
    FrameData& frameData = rg.GetFrameData();

    PGP_MANUAL_ZONEN( __matrixUpdate, "MatrixUpdate" );
    auto view                = scene->registry.view<ModelRenderer, PG::Transform>();
    u32 modelNum             = 0;
    mat4* gpuModelMatricies  = reinterpret_cast<mat4*>( frameData.modelMatricesBuffer.GetMappedPtr() );
    mat4* gpuNormalMatricies = reinterpret_cast<mat4*>( frameData.normalMatricesBuffer.GetMappedPtr() );
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
    PGP_MANUAL_ZONE_END( __matrixUpdate );

    GpuData::SceneGlobals globalData;
    memset( &globalData, 0, sizeof( globalData ) );
    globalData.V                          = scene->camera.GetV();
    globalData.P                          = scene->camera.GetP();
    globalData.VP                         = scene->camera.GetVP();
    globalData.invVP                      = Inverse( scene->camera.GetVP() );
    globalData.cameraPos                  = vec4( scene->camera.position, 1 );
    vec3 skyTint                          = scene->skyTint * powf( 2.0f, scene->skyEVAdjust );
    globalData.cameraExposureAndSkyTint   = vec4( powf( 2.0f, scene->camera.exposure ), skyTint );
    globalData.modelMatriciesBufferIndex  = frameData.modelMatricesBuffer.GetBindlessIndex();
    globalData.normalMatriciesBufferIndex = frameData.normalMatricesBuffer.GetBindlessIndex();
    globalData.r_tonemap                  = r_tonemap.GetUint();

    Lighting::UpdateLights( scene );
    globalData.numLights.x  = Lighting::GetLightCount();
    globalData.lightBuffer  = Lighting::GetLightBufferAddress();
    globalData.ambientColor = vec4( r_ambientScale.GetFloat() * scene->ambientColor, 0 );

#if USING( DEVELOPMENT_BUILD )
    globalData.r_geometryViz    = r_geometryViz.GetUint();
    globalData.r_materialViz    = r_materialViz.GetUint();
    globalData.r_lightingViz    = r_lightingViz.GetUint();
    globalData.r_postProcessing = r_postProcessing.GetBool();
    PackMeshletVizBit( globalData.debug_PackedDvarBools, r_meshletViz.GetBool() );
    PackWireframeBit( globalData.debug_PackedDvarBools, r_wireframe.GetBool() );
    globalData.debug_wireframeData   = r_wireframeColor.GetVec4();
    globalData.debug_wireframeData.w = r_wireframeThickness.GetFloat();
    globalData.debugInt              = r_debugInt.GetInt();
    globalData.debugUint             = r_debugUint.GetUint();
    globalData.debugFloat            = r_debugFloat.GetFloat();

    if ( !r_frustumCullingDebug.GetBool() )
    {
        rg.debugCullingFrustum   = scene->camera.GetFrustum();
        rg.debugCullingCameraPos = scene->camera.position;
    }
    memcpy( globalData.frustumPlanes, rg.debugCullingFrustum.planes, sizeof( vec4 ) * 6 );
    globalData.cullingCameraPos = vec4( rg.debugCullingCameraPos, 1 );
    PackMeshletFrustumCullingBit( globalData.packedCullingDvarBools, r_meshletCulling_frustum.GetBool() );
    PackMeshletBackfaceCullingBit( globalData.packedCullingDvarBools, r_meshletCulling_backface.GetBool() );
#else  // #if USING( DEVELOPMENT_BUILD )
    memcpy( globalData.frustumPlanes, scene->camera.GetFrustum().planes, sizeof( vec4 ) * 6 );
    globalData.cullingCameraPos       = vec4( scene->camera.position, 1 );
    globalData.packedCullingDvarBools = ~0u;
#endif // #else // #if USING( DEVELOPMENT_BUILD )

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
