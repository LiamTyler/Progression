#include "r_scene.hpp"
#include "debug_draw.hpp"
#include "debug_marker.hpp"
#include "r_dvars.hpp"
#include "r_lighting.hpp"
#include "r_pipeline_manager.hpp"

#include "ecs/components/model_renderer.hpp"
#include "ecs/components/transform.hpp"

#include "c_shared/cull_data.h"
#include "c_shared/dvar_defines.h"
#include "c_shared/limits.h"
#include "c_shared/mesh_shading_defines.h"
#include "c_shared/model.h"
#include "c_shared/scene_globals.h"

#define MAX_MODELS_PER_FRAME 131072u
#define MAX_MESHES_PER_FRAME 131072u

namespace PG::Gfx
{

void Init_SceneData()
{
    for ( int i = 0; i < NUM_FRAME_OVERLAP; ++i )
    {
        std::string postfix = "_" + std::to_string( i );

        FrameData& fData = rg.frameData[i];

        BufferCreateInfo modelBufInfo = {};
        modelBufInfo.size             = MAX_MODELS_PER_FRAME * sizeof( mat4 );
        modelBufInfo.bufferUsage      = BufferUsage::STORAGE | BufferUsage::TRANSFER_DST | BufferUsage::DEVICE_ADDRESS;
        modelBufInfo.flags            = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        fData.modelMatricesBuffer     = rg.device.NewBuffer( modelBufInfo, "modelMatrices" + postfix );

        modelBufInfo.size        = MAX_MESHES_PER_FRAME * sizeof( GpuData::MeshCullData );
        fData.meshCullData       = rg.device.NewBuffer( modelBufInfo, "meshCullingInputData" + postfix );
        modelBufInfo.size        = MAX_MESHES_PER_FRAME * sizeof( GpuData::PerObjectData );
        fData.meshDrawDataBuffer = rg.device.NewBuffer( modelBufInfo, "meshDrawData" + postfix );

        BufferCreateInfo sgBufInfo = {};
        sgBufInfo.size             = sizeof( GpuData::SceneGlobals );
        sgBufInfo.bufferUsage      = BufferUsage::UNIFORM | BufferUsage::TRANSFER_DST | BufferUsage::DEVICE_ADDRESS;
        sgBufInfo.flags            = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        fData.sceneGlobalsBuffer   = rg.device.NewBuffer( sgBufInfo, "sceneGlobals" + postfix );

        BufferCreateInfo modelQuantizationBufInfo = {};
        modelQuantizationBufInfo.size             = MAX_MODELS_PER_FRAME * 2 * sizeof( vec3 );
        modelQuantizationBufInfo.bufferUsage      = BufferUsage::STORAGE | BufferUsage::TRANSFER_DST | BufferUsage::DEVICE_ADDRESS;
        modelQuantizationBufInfo.flags  = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        fData.modelDequantizationBuffer = rg.device.NewBuffer( modelQuantizationBufInfo, "modelDequantization" + postfix );
    }
}

void Shutdown_SceneData()
{
    for ( i32 i = 0; i < NUM_FRAME_OVERLAP; ++i )
    {
        rg.frameData[i].meshDrawDataBuffer.Free();
        rg.frameData[i].meshCullData.Free();
        rg.frameData[i].modelMatricesBuffer.Free();
        rg.frameData[i].modelDequantizationBuffer.Free();
        rg.frameData[i].sceneGlobalsBuffer.Free();
    }
}

void UpdateSceneData( Scene* scene )
{
    PG_ASSERT( scene );
    PGP_ZONE_SCOPEDN( "UpdateGPUSceneData" );
    FrameData& frameData = rg.GetFrameData();

    PGP_MANUAL_ZONEN( __matrixUpdate, "MatrixUpdate" );
    auto view               = scene->registry.view<ModelRenderer, PG::Transform>();
    u32 modelNum            = 0;
    mat4* gpuModelMatricies = reinterpret_cast<mat4*>( frameData.modelMatricesBuffer.GetMappedPtr() );
    vec3* gpuModelDQInfo    = reinterpret_cast<vec3*>( frameData.modelDequantizationBuffer.GetMappedPtr() );
    scene->registry.view<ModelRenderer, Transform>().each(
        [&]( ModelRenderer& modelRenderer, PG::Transform& transform )
        {
            if ( modelNum == MAX_MODELS_PER_FRAME )
                return;

            mat4 M                      = transform.PackedMatrixAndScale();
            gpuModelMatricies[modelNum] = M;

            const Model* model               = modelRenderer.model;
            gpuModelDQInfo[2 * modelNum + 0] = model->positionDequantizationInfo.factor;
            gpuModelDQInfo[2 * modelNum + 1] = model->positionDequantizationInfo.globalMin;

            ++modelNum;
        } );
    PGP_MANUAL_ZONE_END( __matrixUpdate );

    GpuData::SceneGlobals globalData;
    memset( &globalData, 0, sizeof( globalData ) );
    globalData.V                            = scene->camera.GetV();
    globalData.P                            = scene->camera.GetP();
    globalData.VP                           = scene->camera.GetVP();
    globalData.invVP                        = Inverse( scene->camera.GetVP() );
    globalData.cameraPos                    = vec4( scene->camera.position, 1 );
    vec3 skyTint                            = scene->skyTint * powf( 2.0f, scene->skyEVAdjust );
    globalData.cameraExposureAndSkyTint     = vec4( powf( 2.0f, scene->camera.exposure ), skyTint );
    globalData.modelDequantizeBufferAddress = frameData.modelDequantizationBuffer.GetDeviceAddress();
    globalData.modelMatriciesBufferAddress  = frameData.modelMatricesBuffer.GetDeviceAddress();
    globalData.r_tonemap                    = r_tonemap.GetUint();

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

                const Mesh& mesh = model->meshes[i];

                GpuData::PerObjectData constants;
                constants.bindlessRangeStart = mesh.bindlessBuffersSlot;
                constants.modelIdx           = modelNum;
                constants.materialIdx        = modelRenderer.materials[i]->GetBindlessIndex();
                constants.numMeshlets        = mesh.numMeshlets;
                meshDrawData[meshNum + i]    = constants;

                GpuData::MeshCullData cData;
                cData.aabb.min           = model->meshAABBs[i].min;
                cData.aabb.max           = model->meshAABBs[i].max;
                cData.modelIndex         = modelNum;
                cData.numMeshlets        = mesh.numMeshlets;
                cData.bindlessRangeStart = mesh.bindlessBuffersSlot;
                cullData[meshNum + i]    = cData;

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
        VkDeviceAddress outputIndirectArgs;
        VkDeviceAddress outputCullResultsBuffer;
        u32 numMeshes;
        u32 _pad;
    };
    ComputePushConstants push;
    push.cullDataBuffer          = data->frameData->meshCullData.GetDeviceAddress();
    push.outputIndirectArgs      = task->GetOutputBuffer( 0 ).GetDeviceAddress();
    push.outputCullResultsBuffer = task->GetOutputBuffer( 1 ).GetDeviceAddress();
    push.numMeshes               = meshNum;
    cmdBuf.PushConstants( push );
    cmdBuf.Dispatch_AutoSized( meshNum, 1, 1 );
}

void ComputeFrustumCullMeshes_SetupIndirectArgs( ComputeTask* task, TGExecuteData* data )
{
    CommandBuffer& cmdBuf = *data->cmdBuf;

    Pipeline* pipeline = PipelineManager::GetPipeline( "setup_culling_indirect_args" );
    cmdBuf.BindPipeline( pipeline );
    cmdBuf.BindGlobalDescriptors();

    struct ComputePushConstants
    {
        VkDeviceAddress argBuffer;
    };
    ComputePushConstants push;
    push.argBuffer = task->GetInputBuffer( 0 ).GetDeviceAddress();
    cmdBuf.PushConstants( push );
    cmdBuf.Dispatch( 1, 1, 1 );
}

void ComputeFrustumCullMeshes_PrefixSum( ComputeTask* task, TGExecuteData* data )
{
    CommandBuffer& cmdBuf = *data->cmdBuf;

    Pipeline* pipeline = PipelineManager::GetPipeline( "frustum_cull_prefix_sum" );
    cmdBuf.BindPipeline( pipeline );
    cmdBuf.BindGlobalDescriptors();

    struct ComputePushConstants
    {
        VkDeviceAddress inputCountBuffer;
        VkDeviceAddress inputBuffer;
    };
    ComputePushConstants push;
    push.inputCountBuffer = task->GetInputBuffer( 0 ).GetDeviceAddress();
    push.inputBuffer      = task->GetInputBuffer( 1 ).GetDeviceAddress();
    cmdBuf.PushConstants( push );
    cmdBuf.DispatchIndirect( task->GetInputBuffer( 0 ), sizeof( DispatchIndirectCommand ) );
}

#if USING( DEVELOPMENT_BUILD )
void ComputeFrustumCullMeshes_Debug( ComputeTask* task, TGExecuteData* data )
{
    CommandBuffer& cmdBuf = *data->cmdBuf;

    Pipeline* pipeline = PipelineManager::GetPipeline( "frustum_cull_meshes_debug" );
    if ( !pipeline->ExtensionsAndFeaturesSupported() )
        return;

    cmdBuf.BindPipeline( pipeline );
    cmdBuf.BindGlobalDescriptors();

    struct ComputePushConstants
    {
        VkDeviceAddress indirectArgsBuffer;
        VkDeviceAddress meshDrawCmdBuffer;
    };
    ComputePushConstants push;
    push.indirectArgsBuffer = task->GetInputBuffer( 0 ).GetDeviceAddress();
    push.meshDrawCmdBuffer  = task->GetInputBuffer( 1 ).GetDeviceAddress();
    cmdBuf.PushConstants( push );
    cmdBuf.Dispatch( 1, 1, 1 );
}
#endif // #if USING( DEVELOPMENT_BUILD )

void CullMeshlets( ComputeTask* task, TGExecuteData* data )
{
    CommandBuffer& cmdBuf = *data->cmdBuf;

    Pipeline* pipeline = PipelineManager::GetPipeline( "cull_meshlets" );
    cmdBuf.BindPipeline( pipeline );
    cmdBuf.BindGlobalDescriptors();

    struct ComputePushConstants
    {
        VkDeviceAddress inputMaxCounts;
        VkDeviceAddress meshCullResults;
        VkDeviceAddress meshCullDataBuffer;
        VkDeviceAddress visibleMeshletBuffer;
        VkDeviceAddress outputMeshletCountBuffer;
    };
    ComputePushConstants push;
    push.inputMaxCounts           = task->GetInputBuffer( 0 ).GetDeviceAddress();
    push.meshCullResults          = task->GetInputBuffer( 1 ).GetDeviceAddress();
    push.meshCullDataBuffer       = data->frameData->meshCullData.GetDeviceAddress();
    push.visibleMeshletBuffer     = task->GetOutputBuffer( 0 ).GetDeviceAddress();
    push.outputMeshletCountBuffer = task->GetOutputBuffer( 1 ).GetDeviceAddress();
    cmdBuf.PushConstants( push );
    cmdBuf.DispatchIndirect( task->GetInputBuffer( 0 ), 2 * sizeof( DispatchIndirectCommand ) );
}

void DrawMeshes_SetupIndirectArgs( ComputeTask* task, TGExecuteData* data )
{
    CommandBuffer& cmdBuf = *data->cmdBuf;

    Pipeline* pipeline = PipelineManager::GetPipeline( "setup_1D_indirect_dispatch" );
    cmdBuf.BindPipeline( pipeline );

    struct ComputePushConstants
    {
        VkDeviceAddress inputCounts;
        VkDeviceAddress outputCounts;
        uint maxDimensionsX;
        uint itemsPerWorkgroup;
    };
    ComputePushConstants push;
    push.inputCounts       = task->GetInputBuffer( 0 ).GetDeviceAddress();
    push.outputCounts      = task->GetInputBuffer( 0 ).GetDeviceAddress() + sizeof( u32 );
    push.maxDimensionsX    = 65535;
    push.itemsPerWorkgroup = 1;
    cmdBuf.PushConstants( push );
    cmdBuf.Dispatch( 1, 1, 1 );
}

void MeshDrawFunc( GraphicsTask* task, TGExecuteData* data )
{
    // return;
    PGP_ZONE_SCOPEDN( "Mesh Pass" );
    CommandBuffer& cmdBuf = *data->cmdBuf;

    bool useDebugShader = false;
#if USING( DEVELOPMENT_BUILD )
    Pipeline* pipeline       = nullptr;
    std::string pipelineName = "litModel";
    useDebugShader           = useDebugShader || r_geometryViz.GetUint();
    useDebugShader           = useDebugShader || r_materialViz.GetUint();
    useDebugShader           = useDebugShader || r_lightingViz.GetUint();
    useDebugShader           = useDebugShader || r_meshletViz.GetBool();
    if ( useDebugShader ) [[unlikely]]
        pipelineName += "_debug";

    if ( r_wireframe.GetBool() ) [[unlikely]]
    {
        pipeline = PipelineManager::GetPipeline( "litModel_debug_wireframe" );
        if ( !pipeline->ExtensionsAndFeaturesSupported() )
        {
            LOG_WARN( "The wireframe shader is not supported on this device. Ignoring wireframe request" );
            pipeline = nullptr;
        }
    }

    if ( !pipeline )
        pipeline = PipelineManager::GetPipeline( pipelineName );
#else  // #if USING( DEVELOPMENT_BUILD )
    Pipeline* pipeline = PipelineManager::GetPipeline( "litModel" );
#endif // #if USING( DEVELOPMENT_BUILD )

    cmdBuf.BindPipeline( pipeline );
    cmdBuf.BindGlobalDescriptors();
    cmdBuf.SetViewport( SceneSizedViewport() );
    cmdBuf.SetScissor( SceneSizedScissor() );

    const Buffer& indirectArgs = task->GetInputBuffer( 0 );
    struct ComputePushConstants
    {
        VkDeviceAddress visibleMeshletBuffer;
        VkDeviceAddress meshDataBuffer;
        VkDeviceAddress workItemsCount;
        u64 _pad;
    };
    ComputePushConstants push;
    push.visibleMeshletBuffer = task->GetInputBuffer( 1 ).GetDeviceAddress();
    push.meshDataBuffer       = data->frameData->meshDrawDataBuffer.GetDeviceAddress();
    push.workItemsCount       = indirectArgs.GetDeviceAddress();
    cmdBuf.PushConstants( push );

    cmdBuf.DrawMeshTasksIndirect( indirectArgs, 1, sizeof( u32 ) );
}

void AddSceneRenderTasks( TaskGraphBuilder& builder, TGBTextureRef& litOutput, TGBTextureRef& sceneDepth )
{
    ComputeTaskBuilder* cTask  = nullptr;
    GraphicsTaskBuilder* gTask = nullptr;

    cTask = builder.AddComputeTask( "FrustumCullMeshes" );
    TGBBufferRef cullingIndirectArgs =
        cTask->AddBufferOutput( "cullingIndirectArgs", VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, 3 * sizeof( DispatchIndirectCommand ), 0 );
    TGBBufferRef meshCullOutputBuffer =
        cTask->AddBufferOutput( "meshCullOutput", VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, sizeof( uvec2 ) * MAX_MESHES_PER_FRAME ); // 1MB
    cTask->SetFunction( ComputeFrustumCullMeshes );

    cTask = builder.AddComputeTask( "FrustumCullMeshes_PrefixSum_SetupIndirectArgs" );
    cTask->AddBufferInput( cullingIndirectArgs );
    cTask->AddBufferOutput( cullingIndirectArgs );
    cTask->SetFunction( ComputeFrustumCullMeshes_SetupIndirectArgs );

#if USING( DEVELOPMENT_BUILD ) && 0
    cTask = builder.AddComputeTask( "FrustumCull_DebugOutput" );
    cTask->AddBufferInput( cullingIndirectArgs );
    cTask->AddBufferInput( meshCullOutputBuffer );
    cTask->SetFunction( ComputeFrustumCullMeshes_Debug );
#endif // #if USING( DEVELOPMENT_BUILD )

    cTask = builder.AddComputeTask( "FrustumCullMeshes_PrefixSum" );
    cTask->AddBufferInput( cullingIndirectArgs, BufferUsage::INDIRECT | BufferUsage::STORAGE );
    cTask->AddBufferInput( meshCullOutputBuffer );
    cTask->AddBufferOutput( meshCullOutputBuffer );
    cTask->SetFunction( ComputeFrustumCullMeshes_PrefixSum );

    cTask = builder.AddComputeTask( "CullMeshlets" );
    cTask->AddBufferInput( cullingIndirectArgs );
    cTask->AddBufferInput( meshCullOutputBuffer );
    TGBBufferRef visibleMeshletBuffer       = cTask->AddBufferOutput( "visibleMeshlet", VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
              sizeof( GpuData::VisibleMeshletPayload ) * PG_MAX_MESHLETS_POST_CULLING ); // 8MB
    TGBBufferRef visibleMeshletIndirectArgs = cTask->AddBufferOutput(
        "visibleMeshletIndirectArgs", VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, sizeof( u32 ) + sizeof( DispatchIndirectCommand ), 0 );
    cTask->SetFunction( CullMeshlets );

    cTask = builder.AddComputeTask( "DrawMeshes_SetupIndirectArgs" );
    cTask->AddBufferInput( visibleMeshletIndirectArgs );
    cTask->AddBufferOutput( visibleMeshletIndirectArgs );
    cTask->SetFunction( DrawMeshes_SetupIndirectArgs );

    gTask = builder.AddGraphicsTask( "DrawMeshes" );
    gTask->AddBufferInput( visibleMeshletIndirectArgs, BufferUsage::INDIRECT | BufferUsage::STORAGE );
    gTask->AddBufferInput( visibleMeshletBuffer );
    litOutput  = gTask->AddColorAttachment( "litOutput", PixelFormat::R16_G16_B16_A16_FLOAT, SIZE_SCENE(), SIZE_SCENE() );
    sceneDepth = gTask->AddDepthAttachment( "sceneDepth", PixelFormat::DEPTH_32_FLOAT, SIZE_SCENE(), SIZE_SCENE(), 1.0f );
    gTask->SetFunction( MeshDrawFunc );
}

} // namespace PG::Gfx
