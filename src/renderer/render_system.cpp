#include "renderer/render_system.hpp"
#include "renderer/graphics_api.hpp"
#include "renderer/render_system.hpp"
#include "renderer/r_globals.hpp"
#include "renderer/r_init.hpp"
#include "renderer/r_renderpasses.hpp"
#include "renderer/r_texture_manager.hpp"
#include "asset/asset_manager.hpp"
#include "glm/glm.hpp"
#include "shaders/c_shared/limits.h"
#include "shaders/c_shared/structs.h"
#include "core/assert.hpp"
#include "core/scene.hpp"
#include "core/window.hpp"
#include "ecs/components/model_renderer.hpp"
#include "ecs/components/transform.hpp"
#include "utils/logger.hpp"
#include <unordered_map>

using namespace PG;
using namespace Gfx;

static Window* s_window;

R_Globals PG::Gfx::r_globals;

Pipeline depthOnlyPipeline;
Pipeline litPipeline;
Pipeline skyboxPipeline;
Pipeline postProcessPipeline;

DescriptorSet postProcessDescriptorSet;
DescriptorSet sceneGlobalDescriptorSet;
DescriptorSet lightsDescriptorSet;
Buffer sceneGlobals;
Buffer s_gpuPointLights;

DescriptorSet bindlessTexturesDescriptorSet;

namespace PG
{
namespace RenderSystem
{

bool Init( bool headless )
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

    if ( !AssetManager::LoadFastFile( "gfx_required" ) )
    {
        return false;
    }

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
    pipelineDesc.renderPass             = GetRenderPass( GFX_RENDER_PASS_DEPTH_PREPASS );
    pipelineDesc.vertexDescriptor       = VertexInputDescriptor::Create( 1, bindingDescs, 1, attribDescs );
    pipelineDesc.rasterizerInfo.winding = WindingOrder::COUNTER_CLOCKWISE;
    pipelineDesc.shaders[0]             = AssetManager::Get< Shader >( "depthVert" );
    depthOnlyPipeline = r_globals.device.NewGraphicsPipeline( pipelineDesc, "DepthPrepass" );
    
    pipelineDesc.renderPass             = GetRenderPass( GFX_RENDER_PASS_LIT );
    pipelineDesc.depthInfo.compareFunc  = CompareFunction::LEQUAL;
    pipelineDesc.depthInfo.depthWriteEnabled = false;
    pipelineDesc.vertexDescriptor       = VertexInputDescriptor::Create( 3, bindingDescs, 3, attribDescs );
    pipelineDesc.rasterizerInfo.winding = WindingOrder::COUNTER_CLOCKWISE;
    pipelineDesc.shaders[0]             = AssetManager::Get< Shader >( "litVert" );
    pipelineDesc.shaders[1]             = AssetManager::Get< Shader >( "litFrag" );
    litPipeline = r_globals.device.NewGraphicsPipeline( pipelineDesc, "Lit" );
    
    // pipelineDesc.renderPass             = GetRenderPass( GFX_RENDER_PASS_SKYBOX );
    // pipelineDesc.vertexDescriptor       = VertexInputDescriptor::Create( 1, bindingDescs, 1, attribDescs );
    // pipelineDesc.rasterizerInfo.winding = WindingOrder::COUNTER_CLOCKWISE;
    // pipelineDesc.shaders[0]             = AssetManager::Get< Shader >( "depthVert" );
    // //pipelineDesc.shaders[1]             = AssetManager::Get< Shader >( "depthFrag" );
    // skyboxPipeline = r_globals.device.NewGraphicsPipeline( pipelineDesc, "DepthPrepass" );
    
    pipelineDesc.renderPass             = GetRenderPass( GFX_RENDER_PASS_POST_PROCESS );
    pipelineDesc.depthInfo.depthTestEnabled  = false;
    pipelineDesc.depthInfo.depthWriteEnabled = false;
    pipelineDesc.rasterizerInfo.winding = WindingOrder::COUNTER_CLOCKWISE;
    pipelineDesc.vertexDescriptor       = VertexInputDescriptor::Create( 0, bindingDescs, 0, attribDescs );
    pipelineDesc.shaders[0]             = AssetManager::Get< Shader >( "postProcessVert" );
    pipelineDesc.shaders[1]             = AssetManager::Get< Shader >( "postProcessFrag" );
    postProcessPipeline = r_globals.device.NewGraphicsPipeline( pipelineDesc, "PostProcess" );


    // BUFFERS + IMAGES
    sceneGlobals = r_globals.device.NewBuffer( sizeof( GPU::SceneGlobals ), BUFFER_TYPE_UNIFORM, MEMORY_TYPE_HOST_VISIBLE, "scene globals ubo" );
    sceneGlobals.Map();
    s_gpuPointLights = r_globals.device.NewBuffer( PG_MAX_NUM_GPU_POINT_LIGHTS * sizeof( GPU::PointLight ), BUFFER_TYPE_UNIFORM, MEMORY_TYPE_HOST_VISIBLE, "point lights ubo" );
    s_gpuPointLights.Map();

    // DESCRIPTOR SETS
    std::vector< VkDescriptorImageInfo > imgDescriptors;
    std::vector< VkDescriptorBufferInfo > bufferDescriptors;
    std::vector< VkWriteDescriptorSet > writeDescriptorSets;

    postProcessDescriptorSet = r_globals.descriptorPool.NewDescriptorSet( postProcessPipeline.GetResourceLayout()->sets[0] );
    imgDescriptors =
    {
        DescriptorImageInfo( r_globals.colorTex, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ),
    };
    writeDescriptorSets =
    {
        WriteDescriptorSet( postProcessDescriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &imgDescriptors[0] ),
    };
    r_globals.device.UpdateDescriptorSets( static_cast< uint32_t >( writeDescriptorSets.size() ), writeDescriptorSets.data() );
    
    sceneGlobalDescriptorSet = r_globals.descriptorPool.NewDescriptorSet( litPipeline.GetResourceLayout()->sets[PG_SCENE_GLOBALS_BUFFER_SET] );
    bufferDescriptors   = { DescriptorBufferInfo( sceneGlobals ) };
    writeDescriptorSets = { WriteDescriptorSet( sceneGlobalDescriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &bufferDescriptors[0] ) };
    r_globals.device.UpdateDescriptorSets( static_cast< uint32_t >( writeDescriptorSets.size() ), writeDescriptorSets.data() );

    lightsDescriptorSet = r_globals.descriptorPool.NewDescriptorSet( litPipeline.GetResourceLayout()->sets[PG_LIGHTS_SET] );
    bufferDescriptors   = { DescriptorBufferInfo( s_gpuPointLights ) };
    writeDescriptorSets = { WriteDescriptorSet( lightsDescriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &bufferDescriptors[0] ) };
    r_globals.device.UpdateDescriptorSets( static_cast< uint32_t >( writeDescriptorSets.size() ), writeDescriptorSets.data() );

    bindlessTexturesDescriptorSet = r_globals.descriptorPool.NewDescriptorSet( litPipeline.GetResourceLayout()->sets[PG_BINDLESS_TEXTURE_SET] );

    return true;
}


void Shutdown()
{
    r_globals.device.WaitForIdle();

    depthOnlyPipeline.Free();
    litPipeline.Free();
    //skyboxPipeline.Free();
    postProcessPipeline.Free();
    
    sceneGlobals.UnMap();
    sceneGlobals.Free();
    s_gpuPointLights.UnMap();
    s_gpuPointLights.Free();

    R_Shutdown();
}


GPU::MaterialData CPUMaterialToGPU( Material* material )
{
    GPU::MaterialData gpuMaterial;
    gpuMaterial.albedoTint = glm::vec4( material->albedo, 1 );
    gpuMaterial.metalness = material->metalness;
    gpuMaterial.roughness = material->roughness;
    gpuMaterial.albedoMapIndex = material->albedoMap ? material->albedoMap->gpuTexture.GetBindlessArrayIndex() : PG_INVALID_TEXTURE_INDEX;
    gpuMaterial.metalnessMapIndex = material->metalnessMap ? material->metalnessMap->gpuTexture.GetBindlessArrayIndex() : PG_INVALID_TEXTURE_INDEX;
    gpuMaterial.roughnessMapIndex = material->roughnessMap ? material->roughnessMap->gpuTexture.GetBindlessArrayIndex() : PG_INVALID_TEXTURE_INDEX;

    return gpuMaterial;
}


static void UpdateGlobalAndLightBuffers( Scene* scene )
{
    GPU::SceneGlobals globalData;
    globalData.V      = scene->camera.GetV();
    globalData.P      = scene->camera.GetP();
    globalData.VP     = scene->camera.GetVP();
    globalData.invVP  = glm::inverse( scene->camera.GetVP() );
    globalData.cameraPos = glm::vec4( scene->camera.position, 1 );

    memcpy( sceneGlobals.MappedPtr(), &globalData, sizeof( GPU::SceneGlobals ) );
    sceneGlobals.FlushCpuWrites();

    static GPU::PointLight cpuPointLights[PG_MAX_NUM_GPU_POINT_LIGHTS];
    if ( scene->pointLights.size() > PG_MAX_NUM_GPU_POINT_LIGHTS )
    {
        LOG_WARN( "Exceeding limit (%d) of GPU point lights (%d). Ignoring any past limit", PG_MAX_NUM_GPU_POINT_LIGHTS, scene->pointLights.size() );
    }
    uint32_t numPointLights = std::min< uint32_t >( PG_MAX_NUM_GPU_POINT_LIGHTS, static_cast< uint32_t >( scene->pointLights.size() ) );
    for ( uint32_t i = 0; i < numPointLights; ++i )
    {
        const PointLight& light = scene->pointLights[i];
        cpuPointLights[i].positionAndRadius = glm::vec4( light.position, light.radius );
        cpuPointLights[i].color = glm::vec4( light.intensity * light.color, 0 );
    }
    memcpy( s_gpuPointLights.MappedPtr(), cpuPointLights, numPointLights * sizeof( GPU::PointLight ) );
    s_gpuPointLights.FlushCpuWrites();
}


void Render( Scene* scene )
{
    PG_ASSERT( scene != nullptr );
    
    // auto chaletTex = AssetManager::Get< GfxImage >( "chalet" );
    // std::vector< VkDescriptorImageInfo > imgDescriptors =
    // {
    //     DescriptorImageInfo( chaletTex->gpuTexture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ),
    // };
    // std::vector< VkWriteDescriptorSet > writeDescriptorSets =
    // {
    //     WriteDescriptorSet( litDescriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &imgDescriptors[0] ),
    // };
    // r_globals.device.UpdateDescriptorSets( static_cast< uint32_t >( writeDescriptorSets.size() ), writeDescriptorSets.data() );
    Gfx::TextureManager::UpdateDescriptors( bindlessTexturesDescriptorSet );

    auto swapChainImageIndex = r_globals.swapchain.AcquireNextImage( r_globals.presentCompleteSemaphore );

    auto& cmdBuf = r_globals.graphicsCommandBuffer;
    cmdBuf.BeginRecording();
    PG_PROFILE_GPU_RESET( cmdBuf );

    PG_PROFILE_GPU_START( cmdBuf, "Frame" );

    UpdateGlobalAndLightBuffers( scene );

    // DEPTH
    cmdBuf.BeginRenderPass( GetRenderPass( GFX_RENDER_PASS_DEPTH_PREPASS ), *GetFramebuffer( GFX_RENDER_PASS_DEPTH_PREPASS ) );
    cmdBuf.BindPipeline( &depthOnlyPipeline );
    cmdBuf.BindDescriptorSet( sceneGlobalDescriptorSet, PG_SCENE_GLOBALS_BUFFER_SET );
    cmdBuf.SetViewport( FullScreenViewport() );
    cmdBuf.SetScissor( FullScreenScissor() );
    glm::mat4 VP = scene->camera.GetVP();
    scene->registry.view< ModelRenderer, Transform >().each( [&]( ModelRenderer& renderer, Transform& transform )
    {
        const auto& model = renderer.model;
        auto M = transform.GetModelMatrix();
        cmdBuf.PushConstants( 0, sizeof( glm::mat4 ), &M[0][0] );
        
        cmdBuf.BindVertexBuffer( model->vertexBuffer, model->gpuPositionOffset, 0 );
        cmdBuf.BindIndexBuffer(  model->indexBuffer );
        
        for ( size_t i = 0; i < model->meshes.size(); ++i )
        {
            const auto& mesh = model->meshes[i];
            PG_DEBUG_MARKER_INSERT( cmdBuf, "Draw \"" + model->name + "\" : \"" + mesh.name + "\"", glm::vec4( 0 ) );
            cmdBuf.DrawIndexed( mesh.startIndex, mesh.numIndices, mesh.startVertex );
        }
    });
    cmdBuf.EndRenderPass();

    // LIT
    cmdBuf.BeginRenderPass( GetRenderPass( GFX_RENDER_PASS_LIT ), *GetFramebuffer( GFX_RENDER_PASS_LIT ) );
    cmdBuf.BindPipeline( &litPipeline );
    cmdBuf.SetViewport( FullScreenViewport() );
    cmdBuf.SetScissor( FullScreenScissor() );
    cmdBuf.BindDescriptorSet( sceneGlobalDescriptorSet, PG_SCENE_GLOBALS_BUFFER_SET );
    cmdBuf.BindDescriptorSet( bindlessTexturesDescriptorSet, PG_BINDLESS_TEXTURE_SET );
    cmdBuf.BindDescriptorSet( lightsDescriptorSet, PG_LIGHTS_SET );
    scene->registry.view< ModelRenderer, Transform >().each( [&]( ModelRenderer& modelRenderer, Transform& transform )
        {
            const auto& model = modelRenderer.model;
            
            auto M = transform.GetModelMatrix();
            auto N = glm::transpose( glm::inverse( M ) );
            GPU::PerObjectData perObjData{ M, N };
            cmdBuf.PushConstants( 0, sizeof( GPU::PerObjectData ), &perObjData );

            cmdBuf.BindVertexBuffer( model->vertexBuffer, model->gpuPositionOffset, 0 );
            cmdBuf.BindVertexBuffer( model->vertexBuffer, model->gpuNormalOffset, 1 );
            cmdBuf.BindVertexBuffer( model->vertexBuffer, model->gpuTexCoordOffset, 2 );
            cmdBuf.BindIndexBuffer( model->indexBuffer );

            for ( size_t i = 0; i < model->meshes.size(); ++i )
            {
                const Mesh& mesh   = modelRenderer.model->meshes[i];
                Material* material = modelRenderer.materials[i];
                GPU::MaterialData gpuMaterial = CPUMaterialToGPU( material );
                cmdBuf.PushConstants( 128, sizeof( gpuMaterial ), &gpuMaterial );
                PG_DEBUG_MARKER_INSERT( cmdBuf, "Draw \"" + model->name + "\" : \"" + mesh.name + "\"", glm::vec4( 0 ) );
                cmdBuf.DrawIndexed( mesh.startIndex, mesh.numIndices, mesh.startVertex );
            }
        });
    cmdBuf.EndRenderPass();
    
    // POST
    cmdBuf.BeginRenderPass( GetRenderPass( GFX_RENDER_PASS_POST_PROCESS ), r_globals.swapchainFramebuffers[swapChainImageIndex] );
    cmdBuf.BindPipeline( &postProcessPipeline );
    cmdBuf.SetViewport( FullScreenViewport() );
    cmdBuf.SetScissor( FullScreenScissor() );
    cmdBuf.BindDescriptorSet( postProcessDescriptorSet, 0 );
    cmdBuf.Draw( 0, 6 );
    cmdBuf.EndRenderPass();

    PG_PROFILE_GPU_END( cmdBuf, "Frame" );

    cmdBuf.EndRecording();
    r_globals.device.SubmitRenderCommands( 1, &cmdBuf );
    r_globals.device.SubmitFrame( swapChainImageIndex );

    PG_PROFILE_GPU_GET_RESULTS();
}


} // namespace RenderSystem
} // namespace PG
