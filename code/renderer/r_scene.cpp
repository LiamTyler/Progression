#include "r_scene.hpp"
#include "debug_draw.hpp"
#include "r_dvars.hpp"
#include "r_lighting.hpp"
#include "r_pipeline_manager.hpp"

#include "ecs/components/model_renderer.hpp"
#include "ecs/components/transform.hpp"

#include "c_shared/cull_data.h"
#include "c_shared/dvar_defines.h"
#include "c_shared/mesh_shading_defines.h"
#include "c_shared/scene_globals.h"

#define MAX_MODELS_PER_FRAME 65536u
#define MAX_MESHES_PER_FRAME 65536u

namespace PG::Gfx
{

struct LocalFrameData
{
    int x;
};

static LocalFrameData s_localFrameDatas[NUM_FRAME_OVERLAP];

inline LocalFrameData& LocalData() { return s_localFrameDatas[Gfx::rg.currentFrameIdx]; }

void Init_SceneData()
{
    for ( int i = 0; i < NUM_FRAME_OVERLAP; ++i )
    {
        FrameData& fData = rg.frameData[i];

        BufferCreateInfo modelBufInfo = {};
        modelBufInfo.size             = MAX_MODELS_PER_FRAME * sizeof( mat4 );
        modelBufInfo.bufferUsage      = BufferUsage::STORAGE | BufferUsage::TRANSFER_DST | BufferUsage::DEVICE_ADDRESS;
        modelBufInfo.flags            = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        fData.modelMatricesBuffer     = rg.device.NewBuffer( modelBufInfo, "modelMatricesBuffer" );
        fData.normalMatricesBuffer    = rg.device.NewBuffer( modelBufInfo, "normalMatricesBuffer" );

        modelBufInfo.size        = MAX_MESHES_PER_FRAME * sizeof( GpuData::MeshCullData );
        fData.meshCullData       = rg.device.NewBuffer( modelBufInfo, "meshCullData" );
        modelBufInfo.size        = MAX_MESHES_PER_FRAME * sizeof( GpuData::PerObjectData );
        fData.meshDrawDataBuffer = rg.device.NewBuffer( modelBufInfo, "meshDrawDataBuffer" );

        BufferCreateInfo sgBufInfo = {};
        sgBufInfo.size             = sizeof( GpuData::SceneGlobals );
        sgBufInfo.bufferUsage      = BufferUsage::UNIFORM | BufferUsage::TRANSFER_DST | BufferUsage::DEVICE_ADDRESS;
        sgBufInfo.flags            = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        fData.sceneGlobalsBuffer   = rg.device.NewBuffer( sgBufInfo, "sceneGlobalsBuffer" );
    }
}

void Shutdown_SceneData()
{
    for ( i32 i = 0; i < NUM_FRAME_OVERLAP; ++i )
    {
        rg.frameData[i].meshCullData.Free();
        rg.frameData[i].meshDrawDataBuffer.Free();
        rg.frameData[i].modelMatricesBuffer.Free();
        rg.frameData[i].normalMatricesBuffer.Free();
        rg.frameData[i].sceneGlobalsBuffer.Free();
    }
}

void UpdateSceneData( Scene* scene )
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

#if USING( INDIRECT_MAIN_DRAW )
void ComputeFrustumCullMeshes( ComputeTask* task, TGExecuteData* data )
{
    CommandBuffer& cmdBuf = *data->cmdBuf;

    GpuData::PerObjectData* meshDrawData = data->frameData->meshDrawDataBuffer.GetMappedPtr<GpuData::PerObjectData>();
    GpuData::MeshCullData* cullData      = data->frameData->meshCullData.GetMappedPtr<GpuData::MeshCullData>();
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

                GpuData::MeshCullData cData;
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

                GpuData::PerObjectData constants;
                constants.bindlessRangeStart = mesh.bindlessBuffersSlot;
                constants.modelIdx           = modelNum;
                constants.materialIdx        = modelRenderer.materials[i]->GetBindlessIndex();
                constants.numMeshlets        = mesh.numMeshlets;
                cmdBuf.PushConstants( constants );

                PG_DEBUG_MARKER_INSERT_CMDBUF( cmdBuf, "Draw '%s' : '%s'", model->GetName(), mesh.name.c_str() );
#if USING( TASK_SHADER_MAIN_DRAW )
                cmdBuf.DrawMeshTasks_AutoSized( mesh.numMeshlets, 1, 1 );
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

void AddSceneRenderTasks( TaskGraphBuilder& builder, TGBTextureRef& litOutput, TGBTextureRef& sceneDepth )
{
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

    litOutput  = gTask->AddColorAttachment( "litOutput", PixelFormat::R16_G16_B16_A16_FLOAT, SIZE_SCENE(), SIZE_SCENE() );
    sceneDepth = gTask->AddDepthAttachment( "sceneDepth", PixelFormat::DEPTH_32_FLOAT, SIZE_SCENE(), SIZE_SCENE(), 1.0f );
    gTask->SetFunction( MeshDrawFunc );

#if USING( DEVELOPMENT_BUILD ) && USING( INDIRECT_MAIN_DRAW )
    cTask = builder.AddComputeTask( "ComputeDraw" );
    cTask->AddBufferInput( indirectCountBuff );
    cTask->AddBufferInput( indirectMeshletDrawBuff );
    cTask->SetFunction( ComputeFrustumCullMeshes_Debug );
#endif // #if USING( DEVELOPMENT_BUILD ) && USING( INDIRECT_MAIN_DRAW )
}

} // namespace PG::Gfx
