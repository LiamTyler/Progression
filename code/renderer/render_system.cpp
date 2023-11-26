#include "renderer/render_system.hpp"
#include "renderer/debug_ui.hpp"
#include "renderer/graphics_api.hpp"
#include "renderer/rendergraph/r_rendergraph.hpp"
#include "renderer/render_system.hpp"
#include "renderer/r_dvars.hpp"
#include "renderer/r_globals.hpp"
#include "renderer/r_init.hpp"
#include "renderer/r_texture_manager.hpp"
#include "asset/asset_manager.hpp"
#include "shaders/c_shared/limits.h"
#include "shaders/c_shared/structs.h"
#include "shared/assert.hpp"
#include "core/dvars.hpp"
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

DescriptorSetLayout bindlessTexturesDescriptorSetLayout;
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
        data.sceneConstantBuffer = rg.device.NewBuffer( sizeof( GpuData::SceneGlobals ), BUFFER_TYPE_UNIFORM, MEMORY_TYPE_HOST_VISIBLE | MEMORY_TYPE_HOST_COHERENT, "sceneConstantsUBO" + std::to_string( i ) );
        data.sceneConstantBuffer.Map();
        
        std::vector<VkDescriptorImageInfo> imgDescriptors;
        std::vector<VkDescriptorBufferInfo> bufferDescriptors;
        std::vector<VkWriteDescriptorSet> writeDescriptorSets;

        data.sceneConstantDescSet = rg.descriptorPool.NewDescriptorSet( litPipeline.GetResourceLayout()->sets[PG_SCENE_GLOBALS_DESCRIPTOR_SET] );
        bufferDescriptors   = { DescriptorBufferInfo( data.sceneConstantBuffer ) };
        writeDescriptorSets = { WriteDescriptorSet_Buffer( data.sceneConstantDescSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, PG_SCENE_CONSTS_BINDING_SLOT, &bufferDescriptors[0] ), };
        rg.device.UpdateDescriptorSets( writeDescriptorSets );

        data.skyboxDescSet = rg.descriptorPool.NewDescriptorSet( skyboxPipeline.GetResourceLayout()->sets[0] );
        imgDescriptors      = { DescriptorImageInfoNull(), DescriptorImageInfoNull(), DescriptorImageInfoNull() };
        writeDescriptorSets = WriteDescriptorSet_Images( data.skyboxDescSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imgDescriptors );
        rg.device.UpdateDescriptorSets( writeDescriptorSets );
        s_skyboxTextures[0] = nullptr;

        data.lightBuffer = rg.device.NewBuffer( PG_MAX_NUM_LIGHTS * sizeof( GpuData::PackedLight ), BUFFER_TYPE_STORAGE, MEMORY_TYPE_HOST_VISIBLE | MEMORY_TYPE_HOST_COHERENT, "lightSSBO" + std::to_string( i ) );
        data.lightBuffer.Map();
        data.lightsDescSet = rg.descriptorPool.NewDescriptorSet( litPipeline.GetResourceLayout()->sets[PG_LIGHT_DESCRIPTOR_SET] );
        bufferDescriptors   = { DescriptorBufferInfo( data.lightBuffer ) };
        writeDescriptorSets = { WriteDescriptorSet_Buffer( data.lightsDescSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, &bufferDescriptors[0] ), };
        rg.device.UpdateDescriptorSets( writeDescriptorSets );

        data.lightingAuxTexturesDescSet = rg.descriptorPool.NewDescriptorSet( litPipeline.GetResourceLayout()->sets[PG_LIGHTING_AUX_DESCRIPTOR_SET] );
        imgDescriptors      = { DescriptorImageInfoNull(), DescriptorImageInfoNull(), DescriptorImageInfo( AssetManager::Get<GfxImage>( "brdfLUT" )->gpuTexture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ) };
        writeDescriptorSets = WriteDescriptorSet_Images( data.lightingAuxTexturesDescSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imgDescriptors );
        rg.device.UpdateDescriptorSets( writeDescriptorSets );

        data.postProcessDescSet = rg.descriptorPool.NewDescriptorSet( postProcessPipeline.GetResourceLayout()->sets[PG_LIGHTING_AUX_DESCRIPTOR_SET] );
        imgDescriptors      = { DescriptorImageInfo( s_renderGraph.GetPhysicalResource( "litOutput", i )->texture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ), };
        writeDescriptorSets = WriteDescriptorSet_Images( data.postProcessDescSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imgDescriptors );
        rg.device.UpdateDescriptorSets( writeDescriptorSets );
    }
}


static void ShutdownPerFrameData()
{
    for ( int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
    {
        FrameData& data = rg.frameData[i];
        data.sceneConstantBuffer.Free();
        data.lightBuffer.Free();
    }
}


static void InitPipelines()
{
    VertexBindingDescriptor bindingDescs[] =
    {
        VertexBindingDescriptor( 0, sizeof( glm::vec3 ) ),
        VertexBindingDescriptor( 1, sizeof( glm::vec3 ) ),
        VertexBindingDescriptor( 2, sizeof( glm::vec4 ) ),
        VertexBindingDescriptor( 3, sizeof( glm::vec2 ) ),
    };

    VertexAttributeDescriptor attribDescs[] =
    {
        VertexAttributeDescriptor( 0, 0, BufferDataType::FLOAT3, 0 ),
        VertexAttributeDescriptor( 1, 1, BufferDataType::FLOAT3, 0 ),
        VertexAttributeDescriptor( 2, 2, BufferDataType::FLOAT4, 0 ),
        VertexAttributeDescriptor( 3, 3, BufferDataType::FLOAT2, 0 ),
    };

    PipelineDescriptor pipelineDesc{};
    pipelineDesc.dynamicAttachmentInfo  = s_renderGraph.GetPipelineAttachmentInfo( "depth_prepass" );
    pipelineDesc.vertexDescriptor       = VertexInputDescriptor::Create( 1, bindingDescs, 1, attribDescs );
    pipelineDesc.rasterizerInfo.winding = WindingOrder::COUNTER_CLOCKWISE;
    pipelineDesc.shaders[0]             = AssetManager::Get< Shader >( "depthVert" );
    depthOnlyPipeline = rg.device.NewGraphicsPipeline( pipelineDesc, "DepthPrepass" );
    
    pipelineDesc.dynamicAttachmentInfo  = s_renderGraph.GetPipelineAttachmentInfo( "lighting" );
    pipelineDesc.depthInfo.compareFunc  = CompareFunction::LEQUAL;
    pipelineDesc.depthInfo.depthWriteEnabled = false;
    pipelineDesc.vertexDescriptor       = VertexInputDescriptor::Create( 4, bindingDescs, 4, attribDescs );
    pipelineDesc.rasterizerInfo.winding = WindingOrder::COUNTER_CLOCKWISE;
    pipelineDesc.shaders[0]             = AssetManager::Get< Shader >( "litVert" );
    pipelineDesc.shaders[1]             = AssetManager::Get< Shader >( "litFrag" );
    litPipeline = rg.device.NewGraphicsPipeline( pipelineDesc, "Lit" );
    
    pipelineDesc.dynamicAttachmentInfo  = s_renderGraph.GetPipelineAttachmentInfo( "skybox" );
    pipelineDesc.vertexDescriptor       = VertexInputDescriptor::Create( 1, bindingDescs, 1, attribDescs );
    pipelineDesc.rasterizerInfo.winding = WindingOrder::COUNTER_CLOCKWISE;
    pipelineDesc.rasterizerInfo.cullFace = CullFace::NONE;
    pipelineDesc.shaders[0]             = AssetManager::Get< Shader >( "skyboxVert" );
    pipelineDesc.shaders[1]             = AssetManager::Get< Shader >( "skyboxFrag" );
    skyboxPipeline = rg.device.NewGraphicsPipeline( pipelineDesc, "Skybox" );
    
    pipelineDesc.dynamicAttachmentInfo       = s_renderGraph.GetPipelineAttachmentInfo( "post_processing" );
    pipelineDesc.depthInfo.depthTestEnabled  = false;
    pipelineDesc.depthInfo.depthWriteEnabled = false;
    pipelineDesc.rasterizerInfo.winding = WindingOrder::COUNTER_CLOCKWISE;
    pipelineDesc.rasterizerInfo.cullFace = CullFace::BACK;
    pipelineDesc.vertexDescriptor       = VertexInputDescriptor::Create( 0, nullptr, 0, nullptr );
    pipelineDesc.shaders[0]             = AssetManager::Get< Shader >( "postProcessVert" );
    pipelineDesc.shaders[1]             = AssetManager::Get< Shader >( "postProcessFrag" );
    postProcessPipeline = rg.device.NewGraphicsPipeline( pipelineDesc, "PostProcess" );
}


static void SetupBindlessDescriptorSet()
{
    bindlessTexturesDescriptorSetLayout = {};
    bindlessTexturesDescriptorSetLayout.sampledImageMask |= (1u << 0);
    bindlessTexturesDescriptorSetLayout.arraySizes[0] = UINT32_MAX;

    uint32_t bindingStages[8] = {};
    bindingStages[0] = 1;

    rg.device.RegisterDescriptorSetLayout( bindlessTexturesDescriptorSetLayout, bindingStages );
    bindlessTexturesDescriptorSet = rg.descriptorPool.NewDescriptorSet( bindlessTexturesDescriptorSetLayout, "bindless textures" );
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

    SetupBindlessDescriptorSet();
    InitPerFrameData();

    if ( !UIOverlay::Init( s_renderGraph.GetPipelineAttachmentInfo( "UI_2D" ).colorAttachmentFormats[0] ) )
        return false;

    return true;
}


void Shutdown()
{
    rg.device.WaitForIdle();
    
    bindlessTexturesDescriptorSetLayout.Free();
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


PipelineAttachmentInfo GetPipelineAttachmentInfo( const std::string& name )
{
    return s_renderGraph.GetPipelineAttachmentInfo( name );
}

static uint32_t GetGpuTexIndex( GfxImage* gfxImage )
{
    return gfxImage ? gfxImage->gpuTexture.GetBindlessArrayIndex() : PG_INVALID_TEXTURE_INDEX;
}

GpuData::MaterialData CPUMaterialToGPU( Material* material )
{
    GpuData::MaterialData gpuMaterial;
    gpuMaterial.albedoTint = material->albedoTint;
    gpuMaterial.metalnessTint = material->metalnessTint;
    gpuMaterial.roughnessTint = material->roughnessTint;
    gpuMaterial.emissiveTint = material->emissiveTint;
    gpuMaterial.albedoMetalnessMapIndex = GetGpuTexIndex( material->albedoMetalnessImage );
    gpuMaterial.normalRoughnessMapIndex = GetGpuTexIndex( material->normalRoughnessImage );
    gpuMaterial.emissiveMapIndex = GetGpuTexIndex( material->emissiveImage );

    return gpuMaterial;
}


static void UpdateGPUSceneData( Scene* scene )
{
    const uint32_t maxNumLights = PG_MAX_NUM_LIGHTS;
    size_t totalLights = scene->directionalLights.size() + scene->pointLights.size() + scene->spotLights.size();
    if ( totalLights > maxNumLights )
    {
        LOG_WARN( "Total number of lights in the scene %u exceeds the gpu light buffer size %u. Skipping the exceeding lights",
            (uint32_t)totalLights, maxNumLights );
    }

    Buffer& lightBuffer = rg.frameData[rg.currentFrame].lightBuffer;
    GpuData::PackedLight* gpuLights = reinterpret_cast<GpuData::PackedLight*>( lightBuffer.MappedPtr() );
    uint32_t globalLightIdx = 0;
    for ( uint32_t i = 0; i < (uint32_t)scene->directionalLights.size() && globalLightIdx < maxNumLights; ++i )
    {
        GpuData::PackedLight& l = gpuLights[globalLightIdx];
        l = PackDirectionalLight( scene->directionalLights[i] );
        ++globalLightIdx;
    }
    for ( uint32_t i = 0; i < (uint32_t)scene->spotLights.size() && globalLightIdx < maxNumLights; ++i )
    {
        GpuData::PackedLight& l = gpuLights[globalLightIdx];
        l = PackSpotLight( scene->spotLights[i] );
        ++globalLightIdx;
    }
    for ( uint32_t i = 0; i < (uint32_t)scene->pointLights.size() && globalLightIdx < maxNumLights; ++i )
    {
        GpuData::PackedLight& l = gpuLights[globalLightIdx];
        l = PackPointLight( scene->pointLights[i] );
        ++globalLightIdx;
    }
    //lightBuffer.FlushCpuWrites();

    GpuData::SceneGlobals globalData;
    globalData.V      = scene->camera.GetV();
    globalData.P      = scene->camera.GetP();
    globalData.VP     = scene->camera.GetVP();
    globalData.invVP  = glm::inverse( scene->camera.GetVP() );
    globalData.cameraPos = glm::vec4( scene->camera.position, 1 );
    VEC3 skyTint = scene->skyTint * powf( 2.0f, scene->skyEVAdjust );
    globalData.cameraExposureAndSkyTint = glm::vec4( powf( 2.0f, scene->camera.exposure ), skyTint );
    globalData.lightCountAndPad3.x = globalLightIdx;
    globalData.r_materialViz = r_materialViz.GetUint();
    globalData.r_lightingViz = r_lightingViz.GetUint();
    globalData.r_postProcessing = r_postProcessing.GetBool();
    globalData.r_tonemap = r_tonemap.GetUint();
    globalData.debugInt = r_debugInt.GetInt();
    globalData.debugUint = r_debugUint.GetUint();
    globalData.debugFloat = r_debugFloat.GetFloat();
    globalData.debug3 = 0u;

    Buffer& sceneBuffer = rg.frameData[rg.currentFrame].sceneConstantBuffer;
    memcpy( sceneBuffer.MappedPtr(), &globalData, sizeof( GpuData::SceneGlobals ) );
    //sceneBuffer.FlushCpuWrites();
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

            imgDescriptors = { DescriptorImageInfo( *s_skyboxTextures[rg.currentFrame], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ),
                               DescriptorImageInfo( scene->skyboxIrradiance->gpuTexture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ),
                               DescriptorImageInfo( scene->skyboxReflectionProbe->gpuTexture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ), };
            writeDescriptorSets = WriteDescriptorSet_Images( frameData.skyboxDescSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imgDescriptors );
            rg.device.UpdateDescriptorSets( static_cast< uint32_t >( writeDescriptorSets.size() ), writeDescriptorSets.data() );

            imgDescriptors      = { DescriptorImageInfo( scene->skyboxIrradiance->gpuTexture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ),
                                    DescriptorImageInfo( scene->skyboxReflectionProbe->gpuTexture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ) };
            writeDescriptorSets = WriteDescriptorSet_Images( frameData.lightingAuxTexturesDescSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imgDescriptors );
            rg.device.UpdateDescriptorSets( static_cast< uint32_t >( writeDescriptorSets.size() ), writeDescriptorSets.data() );
        }
    }
    else
    {
        if ( s_skyboxTextures[rg.currentFrame] )
        {
            VkDescriptorImageInfo nullImg = DescriptorImageInfoNull();
            rg.device.UpdateDescriptorSet( WriteDescriptorSet_Image( frameData.skyboxDescSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &nullImg ) );
            rg.device.UpdateDescriptorSet( WriteDescriptorSet_Image( frameData.skyboxDescSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &nullImg ) );
            rg.device.UpdateDescriptorSet( WriteDescriptorSet_Image( frameData.skyboxDescSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &nullImg ) );

            rg.device.UpdateDescriptorSet( WriteDescriptorSet_Image( frameData.lightingAuxTexturesDescSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &nullImg ) );
            rg.device.UpdateDescriptorSet( WriteDescriptorSet_Image( frameData.lightingAuxTexturesDescSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &nullImg ) );
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

    rg.swapchainImageIndex = rg.swapchain.AcquireNextImage( frameData.swapchainImgAvailableSemaphore );

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

    RG_RenderData renderData
    {
        .scene = scene,
        .cmdBuf = &cmdBuf,
        .swapchain = &rg.swapchain,
        .swapchainImageIndex = rg.swapchainImageIndex
    };
    s_renderGraph.Render( renderData );

#if USING( PG_DEBUG_UI )
    UIOverlay::EndFrame();
#endif // #if USING( PG_DEBUG_UI )

    PG_PROFILE_GPU_END( cmdBuf, "Frame" );
    cmdBuf.EndRecording();
    rg.device.SubmitCommandBuffers( 1, &cmdBuf,frameData.swapchainImgAvailableSemaphore, frameData.renderCompleteSemaphore, &frameData.inFlightFence );
    rg.device.SubmitFrameForPresentation( rg.swapchain, rg.swapchainImageIndex, frameData.renderCompleteSemaphore );

    Gfx::Profile::EndFrame();

    rg.currentFrame = (rg.currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}


static void RenderFunc_DepthPass( RenderTask* task, RG_RenderData& renderData )
{
    if ( !renderData.scene )
        return;

    CommandBuffer* cmdBuf = renderData.cmdBuf;
    cmdBuf->BindPipeline( &depthOnlyPipeline );
    cmdBuf->BindDescriptorSet( rg.frameData[rg.currentFrame].sceneConstantDescSet, PG_SCENE_GLOBALS_DESCRIPTOR_SET );
    cmdBuf->SetViewport( SceneSizedViewport() );
    cmdBuf->SetScissor( SceneSizedScissor() );
    glm::mat4 VP = renderData.scene->camera.GetVP();

    renderData.scene->registry.view< ModelRenderer, Transform >().each( [&]( ModelRenderer& modelRenderer, PG::Transform& transform )
    {
        const Model* model = modelRenderer.model;
        auto M = transform.Matrix();
        cmdBuf->PushConstants( 0, sizeof( glm::mat4 ), &M[0][0] );
        cmdBuf->BindVertexBuffer( model->vertexBuffer, model->gpuPositionOffset, 0 );
        cmdBuf->BindIndexBuffer(  model->indexBuffer );
        
        for ( size_t i = 0; i < model->meshes.size(); ++i )
        {
            const Mesh& mesh = model->meshes[i];
            Material* material = modelRenderer.materials[i];
            if ( material->type == MaterialType::DECAL )
                continue;

            PG_DEBUG_MARKER_INSERT_CMDBUF( (*cmdBuf), "Draw \"" + model->name + "\" : \"" + mesh.name + "\"", glm::vec4( 0 ) );
            cmdBuf->DrawIndexed( mesh.startIndex, mesh.numIndices, mesh.startVertex );
        }
    });
}


static void RenderFunc_LitPass( RenderTask* task, RG_RenderData& renderData )
{
    if ( !renderData.scene )
        return;

    CommandBuffer* cmdBuf = renderData.cmdBuf;
    cmdBuf->BindPipeline( &litPipeline );
    cmdBuf->SetViewport( SceneSizedViewport() );
    cmdBuf->SetScissor( SceneSizedScissor() );
    cmdBuf->BindDescriptorSet( rg.frameData[rg.currentFrame].sceneConstantDescSet, PG_SCENE_GLOBALS_DESCRIPTOR_SET );
    cmdBuf->BindDescriptorSet( bindlessTexturesDescriptorSet, PG_BINDLESS_TEXTURE_DESCRIPTOR_SET );
    cmdBuf->BindDescriptorSet( rg.frameData[rg.currentFrame].lightsDescSet, PG_LIGHT_DESCRIPTOR_SET );
    cmdBuf->BindDescriptorSet( rg.frameData[rg.currentFrame].lightingAuxTexturesDescSet, PG_LIGHTING_AUX_DESCRIPTOR_SET );

    renderData.scene->registry.view< ModelRenderer, Transform >().each( [&]( ModelRenderer& modelRenderer, PG::Transform& transform )
    {
        const Model* model = modelRenderer.model;
        auto M = transform.Matrix();
        auto N = glm::transpose( glm::inverse( M ) );
        GpuData::PerObjectData perObjData{ M, N };
        cmdBuf->PushConstants( 0, sizeof( GpuData::PerObjectData ), &perObjData );
    
        cmdBuf->BindVertexBuffer( model->vertexBuffer, model->gpuPositionOffset, 0 );
        cmdBuf->BindVertexBuffer( model->vertexBuffer, model->gpuNormalOffset, 1 );
        cmdBuf->BindVertexBuffer( model->vertexBuffer, model->gpuTangentOffset, 2 );
        cmdBuf->BindVertexBuffer( model->vertexBuffer, model->gpuTexCoordOffset, 3 );
        cmdBuf->BindIndexBuffer( model->indexBuffer );
    
        for ( size_t i = 0; i < model->meshes.size(); ++i )
        {
            const Mesh& mesh   = modelRenderer.model->meshes[i];
            Material* material = modelRenderer.materials[i];
            if ( material->type == MaterialType::DECAL )
                continue;

            GpuData::MaterialData gpuMaterial = CPUMaterialToGPU( material );
            cmdBuf->PushConstants( 128, sizeof( gpuMaterial ), &gpuMaterial );
            PG_DEBUG_MARKER_INSERT_CMDBUF( (*cmdBuf), "Draw \"" + model->name + "\" : \"" + mesh.name + "\"", glm::vec4( 0 ) );
            cmdBuf->DrawIndexed( mesh.startIndex, mesh.numIndices, mesh.startVertex );
        }
    });
}


static void RenderFunc_SkyboxPass( RenderTask* task, RG_RenderData& renderData )
{
    if ( !renderData.scene )
        return;

    CommandBuffer* cmdBuf = renderData.cmdBuf;
    cmdBuf->BindPipeline( &skyboxPipeline );
    cmdBuf->SetViewport( SceneSizedViewport() );
    cmdBuf->SetScissor( SceneSizedScissor() );
    cmdBuf->BindDescriptorSet( rg.frameData[rg.currentFrame].skyboxDescSet, 0 );
    
    cmdBuf->BindVertexBuffer( s_cubeVertexBuffer );
    cmdBuf->BindIndexBuffer( s_cubeIndexBuffer, IndexType::UNSIGNED_SHORT );
    GpuData::SkyboxData data;
    data.VP = renderData.scene->camera.GetP() * glm::mat4( glm::mat3( renderData.scene->camera.GetV() ) );
    data.hasTexture = s_skyboxTextures[rg.currentFrame] != nullptr;
    data.tint = renderData.scene->skyTint;
    data.scale = exp2f( renderData.scene->skyEVAdjust );
    data.r_skyboxViz = r_skyboxViz.GetUint();
    data.r_skyboxReflectionMipLevel = r_skyboxReflectionMipLevel.GetFloat();
    cmdBuf->PushConstants( 0, sizeof( data ), &data );
    cmdBuf->DrawIndexed( 0, 36 );
}


static void RenderFunc_PostProcessPass( RenderTask* task, RG_RenderData& renderData )
{
    if ( !renderData.scene )
        return;

    CommandBuffer* cmdBuf = renderData.cmdBuf;
    cmdBuf->BindPipeline( &postProcessPipeline );
    cmdBuf->SetViewport( DisplaySizedViewport() );
    cmdBuf->SetScissor( DisplaySizedScissor() );
    cmdBuf->BindDescriptorSet( rg.frameData[rg.currentFrame].sceneConstantDescSet, PG_SCENE_GLOBALS_DESCRIPTOR_SET );
    cmdBuf->BindDescriptorSet( rg.frameData[rg.currentFrame].postProcessDescSet, PG_LIGHTING_AUX_DESCRIPTOR_SET );
    cmdBuf->Draw( 0, 6 );
}


static void RenderFunc_UI2D( RenderTask* task, RG_RenderData& renderData )
{
    CommandBuffer* cmdBuf = renderData.cmdBuf;
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
    task->AddSwapChainOutput();
    task->SetRenderFunction( RenderFunc_PostProcessPass );

    task = builder.AddTask( "UI_2D" );
    task->AddSwapChainOutput();
    task->SetRenderFunction( RenderFunc_UI2D );
    
    RenderGraphCompileInfo compileInfo;
    compileInfo.sceneWidth = width;
    compileInfo.sceneHeight = height;
    compileInfo.displayWidth = rg.swapchain.GetWidth();
    compileInfo.displayHeight = rg.swapchain.GetHeight();
    compileInfo.swapchainFormat = rg.swapchain.GetFormat();
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
