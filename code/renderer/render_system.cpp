#include "renderer/render_system.hpp"
#include "renderer/debug_ui.hpp"
#include "renderer/graphics_api.hpp"
#include "renderer/render_system.hpp"
#include "renderer/r_globals.hpp"
#include "renderer/r_init.hpp"
#include "renderer/rendergraph/r_rendergraph.hpp"
#include "renderer/r_texture_manager.hpp"
#include "asset/asset_manager.hpp"
#include "shaders/c_shared/limits.h"
#include "shaders/c_shared/structs.h"
#include "shared/assert.hpp"
#include "core/feature_defines.hpp"
#include "core/scene.hpp"
#include "core/window.hpp"
#include "ecs/components/model_renderer.hpp"
#include "ecs/components/transform.hpp"
#include "shared/logger.hpp"
#include "ui/ui_system.hpp"

using namespace PG;
using namespace Gfx;

static Window* s_window;

Pipeline depthOnlyPipeline;
Pipeline litPipeline;
Pipeline skyboxPipeline;
Pipeline postProcessPipeline;

Buffer s_cubeVertexBuffer;
Buffer s_cubeIndexBuffer;

DescriptorSet bindlessTexturesDescriptorSet;
Texture* s_skyboxTextures[MAX_FRAMES_IN_FLIGHT];

RenderGraph s_renderGraph;

namespace PG
{
namespace RenderSystem
{

static bool InitRenderGraph( int width, int height );


static void InitPerFrameData()
{
    for ( int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
    {
        FrameData& data = rg.frameData[i];
        data.sceneConstantBuffer = rg.device.NewBuffer( sizeof( GpuData::SceneGlobals ), BUFFER_TYPE_UNIFORM, MEMORY_TYPE_HOST_VISIBLE, "sceneConstantsUBO" + std::to_string( i ) );
        data.sceneConstantBuffer.Map();
        
        std::vector<VkDescriptorImageInfo> imgDescriptors;
        std::vector<VkDescriptorBufferInfo> bufferDescriptors;
        std::vector<VkWriteDescriptorSet> writeDescriptorSets;

        data.postProcessDescSet = rg.descriptorPool.NewDescriptorSet( postProcessPipeline.GetResourceLayout()->sets[0] );
        imgDescriptors      = { DescriptorImageInfo( s_renderGraph.GetPhysicalResource( "litOutput", i )->texture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ), };
        writeDescriptorSets = { WriteDescriptorSet_Image( data.postProcessDescSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &imgDescriptors[0] ), };
        rg.device.UpdateDescriptorSets( static_cast< uint32_t >( writeDescriptorSets.size() ), writeDescriptorSets.data() );
    
        data.sceneConstantDescSet = rg.descriptorPool.NewDescriptorSet( litPipeline.GetResourceLayout()->sets[PG_SCENE_GLOBALS_BUFFER_SET] );
        bufferDescriptors   = { DescriptorBufferInfo( data.sceneConstantBuffer ) };
        writeDescriptorSets = { WriteDescriptorSet_Buffer( data.sceneConstantDescSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &bufferDescriptors[0] ) };
        rg.device.UpdateDescriptorSets( static_cast< uint32_t >( writeDescriptorSets.size() ), writeDescriptorSets.data() );

        data.skyboxDescSet = rg.descriptorPool.NewDescriptorSet( skyboxPipeline.GetResourceLayout()->sets[0] );
        imgDescriptors      = { DescriptorImageInfoNull() };
        writeDescriptorSets = { WriteDescriptorSet_Image( data.skyboxDescSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &imgDescriptors[0] ) };
        rg.device.UpdateDescriptorSets( static_cast< uint32_t >( writeDescriptorSets.size() ), writeDescriptorSets.data() );
        s_skyboxTextures[0] = nullptr;
    }
}


static void ShutdownPerFrameData()
{
    for ( int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
    {
        FrameData& data = rg.frameData[i];
        data.sceneConstantBuffer.Free();
    }
}


static void InitPipelines()
{
    VertexBindingDescriptor bindingDescs[] =
    {
        VertexBindingDescriptor( 0, sizeof( glm::vec3 ) ),
        VertexBindingDescriptor( 1, sizeof( glm::vec3 ) ),
        VertexBindingDescriptor( 2, sizeof( glm::vec2 ) ),
    };

    VertexAttributeDescriptor attribDescs[] =
    {
        VertexAttributeDescriptor( 0, 0, BufferDataType::FLOAT3, 0 ),
        VertexAttributeDescriptor( 1, 1, BufferDataType::FLOAT3, 0 ),
        VertexAttributeDescriptor( 2, 2, BufferDataType::FLOAT2, 0 ),
    };

    PipelineDescriptor pipelineDesc;
    pipelineDesc.renderPass             = s_renderGraph.GetRenderPass( "depth_prepass" );
    pipelineDesc.vertexDescriptor       = VertexInputDescriptor::Create( 1, bindingDescs, 1, attribDescs );
    pipelineDesc.rasterizerInfo.winding = WindingOrder::COUNTER_CLOCKWISE;
    pipelineDesc.shaders[0]             = AssetManager::Get< Shader >( "depthVert" );
    depthOnlyPipeline = rg.device.NewGraphicsPipeline( pipelineDesc, "DepthPrepass" );
    
    pipelineDesc.renderPass             = s_renderGraph.GetRenderPass( "lighting" );
    pipelineDesc.depthInfo.compareFunc  = CompareFunction::LEQUAL;
    pipelineDesc.depthInfo.depthWriteEnabled = false;
    pipelineDesc.vertexDescriptor       = VertexInputDescriptor::Create( 3, bindingDescs, 3, attribDescs );
    pipelineDesc.rasterizerInfo.winding = WindingOrder::COUNTER_CLOCKWISE;
    pipelineDesc.shaders[0]             = AssetManager::Get< Shader >( "litVert" );
    pipelineDesc.shaders[1]             = AssetManager::Get< Shader >( "litFrag" );
    litPipeline = rg.device.NewGraphicsPipeline( pipelineDesc, "Lit" );
    
    pipelineDesc.renderPass             = s_renderGraph.GetRenderPass( "skybox" );
    pipelineDesc.vertexDescriptor       = VertexInputDescriptor::Create( 1, bindingDescs, 1, attribDescs );
    pipelineDesc.rasterizerInfo.winding = WindingOrder::COUNTER_CLOCKWISE;
    pipelineDesc.rasterizerInfo.cullFace = CullFace::NONE;
    pipelineDesc.shaders[0]             = AssetManager::Get< Shader >( "skyboxVert" );
    pipelineDesc.shaders[1]             = AssetManager::Get< Shader >( "skyboxFrag" );
    skyboxPipeline = rg.device.NewGraphicsPipeline( pipelineDesc, "Skybox" );
    
    pipelineDesc.renderPass             = s_renderGraph.GetRenderPass( "post_processing" );
    pipelineDesc.depthInfo.depthTestEnabled  = false;
    pipelineDesc.depthInfo.depthWriteEnabled = false;
    pipelineDesc.rasterizerInfo.winding = WindingOrder::COUNTER_CLOCKWISE;
    pipelineDesc.rasterizerInfo.cullFace = CullFace::BACK;
    pipelineDesc.vertexDescriptor       = VertexInputDescriptor::Create( 0, nullptr, 0, nullptr );
    pipelineDesc.shaders[0]             = AssetManager::Get< Shader >( "postProcessVert" );
    pipelineDesc.shaders[1]             = AssetManager::Get< Shader >( "postProcessFrag" );
    postProcessPipeline = rg.device.NewGraphicsPipeline( pipelineDesc, "PostProcess" );
}


bool Init( uint32_t sceneWidth, uint32_t sceneHeight, bool headless )
{
    s_window = GetMainWindow();

    if ( headless )
    {
        return R_Init( true );
    }
    else
    {
        if ( !R_Init( false, s_window->Width(), s_window->Height() ) )
        {
            return false;
        }
    }
    
    rg.currentFrame = 0;
    rg.sceneWidth = sceneWidth;
    rg.sceneHeight = sceneHeight;

    if ( !AssetManager::LoadFastFile( "defaults_required" ) || !AssetManager::LoadFastFile( "gfx_required" ) )
    {
        return false;
    }
    
    if ( !InitRenderGraph( sceneWidth, sceneHeight ) )
    {
        return false;
    }

    InitPipelines();

    // BUFFERS + IMAGES
    {
        glm::vec3 verts[] =
        {
            glm::vec3( -1, -1, -1 ),
            glm::vec3( -1,  1, -1 ),
            glm::vec3(  1, -1, -1 ),
            glm::vec3(  1,  1, -1 ),
            glm::vec3( -1, -1,  1 ),
            glm::vec3( -1,  1,  1 ),
            glm::vec3(  1, -1,  1 ),
            glm::vec3(  1,  1,  1 ),
        };
        uint16_t indices[] =
        {
            0, 1, 3, // front
            0, 3, 2,

            2, 3, 7, // right
            2, 7, 6,
        
            6, 7, 5, // back
            6, 5, 4,
        
            4, 5, 1, // left
            4, 1, 0,

            4, 0, 2, // top
            4, 2, 6,
        
            1, 5, 7, // bottom
            1, 7, 3,
        };
        s_cubeVertexBuffer = rg.device.NewBuffer( sizeof( verts ), verts, BUFFER_TYPE_VERTEX, MEMORY_TYPE_DEVICE_LOCAL, "cube vertex buffer" );
        s_cubeIndexBuffer = rg.device.NewBuffer( sizeof( indices ), indices, BUFFER_TYPE_INDEX, MEMORY_TYPE_DEVICE_LOCAL, "cube index buffer" );
    }

    bindlessTexturesDescriptorSet = rg.descriptorPool.NewDescriptorSet( litPipeline.GetResourceLayout()->sets[PG_BINDLESS_TEXTURE_SET] );
    InitPerFrameData();

    if ( !UIOverlay::Init( *s_renderGraph.GetRenderPass( "UI_2D" ) ) )
        return false;

    return true;
}


void Shutdown()
{
    rg.device.WaitForIdle();
    
    UIOverlay::Shutdown();
    s_renderGraph.Free();
    depthOnlyPipeline.Free();
    litPipeline.Free();
    skyboxPipeline.Free();
    postProcessPipeline.Free();
    ShutdownPerFrameData();
    s_cubeIndexBuffer.Free();
    s_cubeVertexBuffer.Free();

    AssetManager::FreeRemainingGpuResources();
    R_Shutdown();
}


void CreateTLAS( Scene* scene )
{
#if USING( PG_RTX )
    auto view = scene->registry.view<ModelRenderer, Transform>();
    std::vector<VkAccelerationStructureInstanceKHR> instanceTransforms;
    instanceTransforms.reserve( view.size_hint() );
    view.each( [&]( ModelRenderer& modelRenderer, Transform& transform )
    {
        VkAccelerationStructureInstanceKHR instance{};
        auto M = glm::transpose( transform.Matrix() );
        std::memcpy( &instance.transform, &M[0][0], sizeof( VkTransformMatrixKHR ) );
        instance.instanceCustomIndex = 0;
        instance.mask = 0xFF;
        instance.instanceShaderBindingTableRecordOffset = 0;
        instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        instance.accelerationStructureReference = modelRenderer.model->blas.GetDeviceAddress();

        instanceTransforms.push_back( instance );
    });

    size_t bufferSize = instanceTransforms.size() * sizeof( VkAccelerationStructureInstanceKHR );
    Buffer instancesBuffer = rg.device.NewBuffer( bufferSize, BUFFER_TYPE_DEVICE_ADDRESS | BUFFER_TYPE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY, MEMORY_TYPE_HOST_VISIBLE | MEMORY_TYPE_HOST_COHERENT, "tlas" );

	VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
	instanceDataDeviceAddress.deviceAddress = instancesBuffer.GetDeviceAddress();

	VkAccelerationStructureGeometryKHR accelerationStructureGeometry{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
	accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	accelerationStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
	accelerationStructureGeometry.geometry.instances.data = instanceDataDeviceAddress;

	VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
	accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    accelerationStructureBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	accelerationStructureBuildGeometryInfo.geometryCount = 1;
	accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

	uint32_t primitive_count = 1;

	VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
	vkGetAccelerationStructureBuildSizesKHR( rg.device.GetHandle(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &accelerationStructureBuildGeometryInfo, &primitive_count, &accelerationStructureBuildSizesInfo );

    scene->tlas = rg.device.NewAccelerationStructure( AccelerationStructureType::TLAS, accelerationStructureBuildSizesInfo.accelerationStructureSize );

    Buffer scratchBuffer = rg.device.NewBuffer( accelerationStructureBuildSizesInfo.buildScratchSize, BUFFER_TYPE_STORAGE | BUFFER_TYPE_DEVICE_ADDRESS, MEMORY_TYPE_DEVICE_LOCAL );

    VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
    accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    accelerationBuildGeometryInfo.dstAccelerationStructure = scene->tlas.GetHandle();
    accelerationBuildGeometryInfo.geometryCount = 1;
    accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
    accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.GetDeviceAddress();

    VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
    accelerationStructureBuildRangeInfo.primitiveCount = 1;
    accelerationStructureBuildRangeInfo.primitiveOffset = 0;
    accelerationStructureBuildRangeInfo.firstVertex = 0;
    accelerationStructureBuildRangeInfo.transformOffset = 0;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

    CommandBuffer cmdBuf = rg.commandPools[GFX_CMD_POOL_TRANSIENT].NewCommandBuffer( "One time Build AS" );
    cmdBuf.BeginRecording( COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT );
    vkCmdBuildAccelerationStructuresKHR( cmdBuf.GetHandle(), 1, &accelerationBuildGeometryInfo, accelerationBuildStructureRangeInfos.data() );
    cmdBuf.EndRecording();
    rg.device.Submit( cmdBuf );
    rg.device.WaitForIdle();
    cmdBuf.Free();
    scratchBuffer.Free();
    instancesBuffer.Free();
#endif // #if USING( PG_RTX )
}


RenderPass* GetRenderPass( const std::string& name )
{
    return s_renderGraph.GetRenderPass( name );
}


GpuData::MaterialData CPUMaterialToGPU( Material* material )
{
    GpuData::MaterialData gpuMaterial;
    gpuMaterial.albedoTint = glm::vec4( material->albedoTint, 1 );
    gpuMaterial.metalnessTint = material->metalnessTint;
    gpuMaterial.albedoMetalnessMapIndex = material->albedoMetalnessImage ? material->albedoMetalnessImage->gpuTexture.GetBindlessArrayIndex() : PG_INVALID_TEXTURE_INDEX;

    return gpuMaterial;
}


static void UpdateGPUSceneData( Scene* scene )
{
    GpuData::SceneGlobals globalData;
    globalData.V      = scene->camera.GetV();
    globalData.P      = scene->camera.GetP();
    globalData.VP     = scene->camera.GetVP();
    globalData.invVP  = glm::inverse( scene->camera.GetVP() );
    globalData.cameraPos = glm::vec4( scene->camera.position, 1 );

    Buffer& buffer = rg.frameData[rg.currentFrame].sceneConstantBuffer;
    memcpy( buffer.MappedPtr(), &globalData, sizeof( GpuData::SceneGlobals ) );
    buffer.FlushCpuWrites();

    //static GpuData::PointLight cpuPointLights[PG_MAX_NUM_GPU_POINT_LIGHTS];
    //if ( scene->pointLights.size() > PG_MAX_NUM_GPU_POINT_LIGHTS )
    //{
    //    LOG_WARN( "Exceeding limit (%d) of GPU point lights (%zu). Ignoring any past limit", PG_MAX_NUM_GPU_POINT_LIGHTS, scene->pointLights.size() );
    //}
    //uint32_t numPointLights = std::min< uint32_t >( PG_MAX_NUM_GPU_POINT_LIGHTS, static_cast< uint32_t >( scene->pointLights.size() ) );
    //for ( uint32_t i = 0; i < numPointLights; ++i )
    //{
    //    const PointLight& light = scene->pointLights[i];
    //    cpuPointLights[i].positionAndRadius = glm::vec4( light.position, light.radius );
    //    cpuPointLights[i].color = glm::vec4( light.intensity * light.color, 0 );
    //}
    //memcpy( s_gpuPointLights.MappedPtr(), cpuPointLights, numPointLights * sizeof( GpuData::PointLight ) );
    //s_gpuPointLights.FlushCpuWrites();
}


static void UpdateSkyboxAndBackground( Scene* scene )
{
    const FrameData& frameData = rg.frameData[rg.currentFrame];
    if ( scene->skybox )
    {
        if ( s_skyboxTextures[rg.currentFrame] != &scene->skybox->gpuTexture )
        {
            s_skyboxTextures[rg.currentFrame] = &scene->skybox->gpuTexture;
            std::vector< VkDescriptorImageInfo > imgDescriptors;
            std::vector< VkWriteDescriptorSet > writeDescriptorSets;

            imgDescriptors      = { DescriptorImageInfo( *s_skyboxTextures[rg.currentFrame], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ), };
            writeDescriptorSets = { WriteDescriptorSet_Image( frameData.skyboxDescSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &imgDescriptors[0] ), };
            rg.device.UpdateDescriptorSets( static_cast< uint32_t >( writeDescriptorSets.size() ), writeDescriptorSets.data() );
        }
    }
    else
    {
        if ( s_skyboxTextures[rg.currentFrame] )
        {
            VkDescriptorImageInfo nullImg = DescriptorImageInfoNull();
            rg.device.UpdateDescriptorSet( WriteDescriptorSet_Image( frameData.skyboxDescSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &nullImg ) );
        }
        s_skyboxTextures[rg.currentFrame] = nullptr;
    }
}


static void UpdateDescriptors()
{
    TextureManager::UpdateDescriptors( bindlessTexturesDescriptorSet );
}


void Render()
{
    FrameData& frameData = rg.frameData[rg.currentFrame];
    frameData.inFlightFence.WaitFor();
    frameData.inFlightFence.Reset();

    UpdateDescriptors();

    rg.swapChainImageIndex = rg.swapchain.AcquireNextImage( frameData.swapchainImgAvailableSemaphore );

    auto& cmdBuf = frameData.graphicsCmdBuf;
    cmdBuf.BeginRecording();
    Gfx::Profile::StartFrame( cmdBuf );
    PG_PROFILE_GPU_START( cmdBuf, "Frame" );

#if USING( PG_DEBUG_UI )
    UIOverlay::BeginFrame();
#endif // #if USING( PG_DEBUG_UI )

    Scene* scene = GetPrimaryScene();
    if ( scene )
    {
        UpdateGPUSceneData( scene );
        UpdateSkyboxAndBackground( scene );
    }

    s_renderGraph.Render( scene, &cmdBuf );

#if USING( PG_DEBUG_UI )
    UIOverlay::EndFrame();
#endif // #if USING( PG_DEBUG_UI )

    PG_PROFILE_GPU_END( cmdBuf, "Frame" );
    cmdBuf.EndRecording();
    rg.device.SubmitCommandBuffers( 1, &cmdBuf,frameData.swapchainImgAvailableSemaphore, frameData.renderCompleteSemaphore, &frameData.inFlightFence );
    rg.device.SubmitFrameForPresentation( rg.swapchain, rg.swapChainImageIndex, frameData.renderCompleteSemaphore );

    Gfx::Profile::EndFrame();

    rg.currentFrame = (rg.currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}


static void RenderFunc_DepthPass( RenderTask* task, Scene* scene, CommandBuffer* cmdBuf )
{
    if ( !scene )
        return;

    cmdBuf->BindPipeline( &depthOnlyPipeline );
    cmdBuf->BindDescriptorSet( rg.frameData[rg.currentFrame].sceneConstantDescSet, PG_SCENE_GLOBALS_BUFFER_SET );
    cmdBuf->SetViewport( SceneSizedViewport() );
    cmdBuf->SetScissor( SceneSizedScissor() );
    glm::mat4 VP = scene->camera.GetVP();

    scene->registry.view< ModelRenderer, Transform >().each( [&]( ModelRenderer& renderer, Transform& transform )
    {
        const Model* model = renderer.model;
        auto M = transform.Matrix();
        cmdBuf->PushConstants( 0, sizeof( glm::mat4 ), &M[0][0] );
        cmdBuf->BindVertexBuffer( model->vertexBuffer, model->gpuPositionOffset, 0 );
        cmdBuf->BindIndexBuffer(  model->indexBuffer );
        
        for ( size_t i = 0; i < model->meshes.size(); ++i )
        {
            const auto& mesh = model->meshes[i];
            PG_DEBUG_MARKER_INSERT_CMDBUF( (*cmdBuf), "Draw \"" + model->name + "\" : \"" + mesh.name + "\"", glm::vec4( 0 ) );
            cmdBuf->DrawIndexed( mesh.startIndex, mesh.numIndices, mesh.startVertex );
        }
    });
}


static void RenderFunc_LitPass( RenderTask* task, Scene* scene, CommandBuffer* cmdBuf )
{
    if ( !scene )
        return;

    cmdBuf->BindPipeline( &litPipeline );
    cmdBuf->SetViewport( SceneSizedViewport() );
    cmdBuf->SetScissor( SceneSizedScissor() );
    cmdBuf->BindDescriptorSet( rg.frameData[rg.currentFrame].sceneConstantDescSet, PG_SCENE_GLOBALS_BUFFER_SET );
    cmdBuf->BindDescriptorSet( bindlessTexturesDescriptorSet, PG_BINDLESS_TEXTURE_SET );

    scene->registry.view< ModelRenderer, Transform >().each( [&]( ModelRenderer& modelRenderer, Transform& transform )
    {
        const Model* model = modelRenderer.model;
        auto M = transform.Matrix();
        auto N = glm::transpose( glm::inverse( M ) );
        GpuData::PerObjectData perObjData{ M, N };
        cmdBuf->PushConstants( 0, sizeof( GpuData::PerObjectData ), &perObjData );
    
        cmdBuf->BindVertexBuffer( model->vertexBuffer, model->gpuPositionOffset, 0 );
        cmdBuf->BindVertexBuffer( model->vertexBuffer, model->gpuNormalOffset, 1 );
        cmdBuf->BindVertexBuffer( model->vertexBuffer, model->gpuTexCoordOffset, 2 );
        cmdBuf->BindIndexBuffer( model->indexBuffer );
    
        for ( size_t i = 0; i < model->meshes.size(); ++i )
        {
            const Mesh& mesh   = modelRenderer.model->meshes[i];
            Material* material = modelRenderer.materials[i];
            GpuData::MaterialData gpuMaterial = CPUMaterialToGPU( material );
            cmdBuf->PushConstants( 128, sizeof( gpuMaterial ), &gpuMaterial );
            PG_DEBUG_MARKER_INSERT_CMDBUF( (*cmdBuf), "Draw \"" + model->name + "\" : \"" + mesh.name + "\"", glm::vec4( 0 ) );
            cmdBuf->DrawIndexed( mesh.startIndex, mesh.numIndices, mesh.startVertex );
        }
    });
}


static void RenderFunc_SkyboxPass( RenderTask* task, Scene* scene, CommandBuffer* cmdBuf )
{
    if ( !scene )
        return;

    cmdBuf->BindPipeline( &skyboxPipeline );
    cmdBuf->SetViewport( SceneSizedViewport() );
    cmdBuf->SetScissor( SceneSizedScissor() );
    cmdBuf->BindDescriptorSet( rg.frameData[rg.currentFrame].skyboxDescSet, 0 );
    
    cmdBuf->BindVertexBuffer( s_cubeVertexBuffer );
    cmdBuf->BindIndexBuffer( s_cubeIndexBuffer, IndexType::UNSIGNED_SHORT );
    glm::mat4 cubeVP = scene->camera.GetP() * glm::mat4( glm::mat3( scene->camera.GetV() ) );
    cmdBuf->PushConstants( 0, sizeof( cubeVP ), &cubeVP );
    GpuData::SkyboxData data;
    data.hasTexture = s_skyboxTextures[rg.currentFrame] != nullptr;
    data.tint = scene->skyTint;
    data.scale = exp2f( scene->skyEVAdjust );
    cmdBuf->PushConstants( 64, sizeof( data ), &data );
    cmdBuf->DrawIndexed( 0, 36 );
}


static void RenderFunc_PostProcessPass( RenderTask* task, Scene* scene, CommandBuffer* cmdBuf )
{
    if ( !scene )
        return;

    cmdBuf->BindPipeline( &postProcessPipeline );
    cmdBuf->SetViewport( DisplaySizedViewport() );
    cmdBuf->SetScissor( DisplaySizedScissor() );
    cmdBuf->BindDescriptorSet( rg.frameData[rg.currentFrame].postProcessDescSet, 0 );
    cmdBuf->Draw( 0, 6 );
}


static void RenderFunc_UI2D( RenderTask* task, Scene* scene, CommandBuffer* cmdBuf )
{
    UI::Render( cmdBuf, &bindlessTexturesDescriptorSet );
#if USING( PG_DEBUG_UI )
    UIOverlay::AddDrawFunction( Profile::DrawResultsOnScreen );
    UIOverlay::Render( *cmdBuf );
#endif // #if USING( PG_DEBUG_UI )
}


static bool InitRenderGraph( int width, int height )
{
    RenderTaskBuilder* task;
    RenderGraphBuilder builder;

    task = builder.AddTask( "depth_prepass" );
    task->AddDepthOutput( "depth", PixelFormat::DEPTH_32_FLOAT, SIZE_SCENE(), SIZE_SCENE(), 1.0f );
    task->SetRenderFunction( RenderFunc_DepthPass );

    task = builder.AddTask( "lighting" );
    task->AddColorOutput( "litOutput", PixelFormat::R16_G16_B16_A16_FLOAT, SIZE_SCENE(), SIZE_SCENE(), 1, 1, 1, glm::vec4( 0 ) );
    task->AddDepthOutput( "depth" );
    task->SetRenderFunction( RenderFunc_LitPass );

    task = builder.AddTask( "skybox" );
    task->AddColorOutput( "litOutput" );
    task->AddDepthOutput( "depth" );
    task->SetRenderFunction( RenderFunc_SkyboxPass );
    
    task = builder.AddTask( "post_processing" );
    task->AddTextureInput( "litOutput" );
    task->AddColorOutput( "finalOutput", PixelFormat::R8_G8_B8_A8_UNORM, SIZE_DISPLAY(), SIZE_DISPLAY(), 1, 1, 1 );
    task->SetRenderFunction( RenderFunc_PostProcessPass );

    task = builder.AddTask( "UI_2D" );
    //task->AddTextureInput( "postProcessOutput" );
    task->AddColorOutput( "finalOutput" );
    task->SetRenderFunction( RenderFunc_UI2D );

    builder.SetBackbufferResource( "finalOutput" );
    
    RenderGraphCompileInfo compileInfo;
    compileInfo.sceneWidth = width;
    compileInfo.sceneHeight = height;
    compileInfo.displayWidth = rg.swapchain.GetWidth();
    compileInfo.displayHeight = rg.swapchain.GetHeight();
    if ( !s_renderGraph.Compile( builder, compileInfo ) )
    {
        LOG_ERR( "Failed to compile task graph" );
        return false;
    }

    //s_renderGraph.Print();
    
    return true;
}

} // namespace RenderSystem
} // namespace PG
