#include "renderer/render_system.hpp"
#include "renderer/graphics_api.hpp"
#include "renderer/render_system.hpp"
#include "renderer/r_globals.hpp"
#include "renderer/r_init.hpp"
#include "renderer/rendergraph/r_rendergraph.hpp"
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

Pipeline depthOnlyPipeline;
Pipeline litPipeline;
Pipeline skyboxPipeline;
Pipeline postProcessPipeline;

DescriptorSet postProcessDescriptorSet;
DescriptorSet sceneGlobalDescriptorSet;
DescriptorSet lightsDescriptorSet;
Buffer sceneGlobals;
Buffer s_gpuPointLights;
Buffer s_cubeVertexBuffer;
Buffer s_cubeIndexBuffer;

DescriptorSet bindlessTexturesDescriptorSet;
DescriptorSet skyboxDescriptorSet;
Texture* skyboxTexture;

RenderGraph s_renderGraph;

namespace PG
{
namespace RenderSystem
{

static bool InitRenderGraph( int width, int height );


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
    r_globals.sceneWidth = sceneWidth;
    r_globals.sceneHeight = sceneHeight;

    if ( !AssetManager::LoadFastFile( "gfx_required" ) )
    {
        return false;
    }
    
    if ( !InitRenderGraph( sceneWidth, sceneHeight ) )
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
    pipelineDesc.renderPass             = &s_renderGraph.GetRenderTask( "depth_prepass" )->renderPass;
    pipelineDesc.vertexDescriptor       = VertexInputDescriptor::Create( 1, bindingDescs, 1, attribDescs );
    pipelineDesc.rasterizerInfo.winding = WindingOrder::COUNTER_CLOCKWISE;
    pipelineDesc.shaders[0]             = AssetManager::Get< Shader >( "depthVert" );
    depthOnlyPipeline = r_globals.device.NewGraphicsPipeline( pipelineDesc, "DepthPrepass" );
    
    pipelineDesc.renderPass             = &s_renderGraph.GetRenderTask( "lighting" )->renderPass;
    pipelineDesc.depthInfo.compareFunc  = CompareFunction::LEQUAL;
    pipelineDesc.depthInfo.depthWriteEnabled = false;
    pipelineDesc.vertexDescriptor       = VertexInputDescriptor::Create( 3, bindingDescs, 3, attribDescs );
    pipelineDesc.rasterizerInfo.winding = WindingOrder::COUNTER_CLOCKWISE;
    pipelineDesc.shaders[0]             = AssetManager::Get< Shader >( "litVert" );
    pipelineDesc.shaders[1]             = AssetManager::Get< Shader >( "litFrag" );
    litPipeline = r_globals.device.NewGraphicsPipeline( pipelineDesc, "Lit" );
    
    pipelineDesc.renderPass             = &s_renderGraph.GetRenderTask( "skybox" )->renderPass;
    pipelineDesc.vertexDescriptor       = VertexInputDescriptor::Create( 1, bindingDescs, 1, attribDescs );
    pipelineDesc.rasterizerInfo.winding = WindingOrder::COUNTER_CLOCKWISE;
    pipelineDesc.rasterizerInfo.cullFace = CullFace::NONE;
    pipelineDesc.shaders[0]             = AssetManager::Get< Shader >( "skyboxVert" );
    pipelineDesc.shaders[1]             = AssetManager::Get< Shader >( "skyboxFrag" );
    skyboxPipeline = r_globals.device.NewGraphicsPipeline( pipelineDesc, "Skybox" );
    
    pipelineDesc.renderPass             = &s_renderGraph.GetRenderTask( "post_processing" )->renderPass;
    pipelineDesc.depthInfo.depthTestEnabled  = false;
    pipelineDesc.depthInfo.depthWriteEnabled = false;
    pipelineDesc.rasterizerInfo.winding = WindingOrder::COUNTER_CLOCKWISE;
    pipelineDesc.rasterizerInfo.cullFace = CullFace::BACK;
    pipelineDesc.vertexDescriptor       = VertexInputDescriptor::Create( 0, bindingDescs, 0, attribDescs );
    pipelineDesc.shaders[0]             = AssetManager::Get< Shader >( "postProcessVert" );
    pipelineDesc.shaders[1]             = AssetManager::Get< Shader >( "postProcessFrag" );
    postProcessPipeline = r_globals.device.NewGraphicsPipeline( pipelineDesc, "PostProcess" );

    // BUFFERS + IMAGES
    {
        sceneGlobals = r_globals.device.NewBuffer( sizeof( GPU::SceneGlobals ), BUFFER_TYPE_UNIFORM, MEMORY_TYPE_HOST_VISIBLE, "scene globals ubo" );
        sceneGlobals.Map();
        s_gpuPointLights = r_globals.device.NewBuffer( PG_MAX_NUM_GPU_POINT_LIGHTS * sizeof( GPU::PointLight ), BUFFER_TYPE_UNIFORM, MEMORY_TYPE_HOST_VISIBLE, "point lights ubo" );
        s_gpuPointLights.Map();

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
        s_cubeVertexBuffer = r_globals.device.NewBuffer( sizeof( verts ), verts, BUFFER_TYPE_VERTEX, MEMORY_TYPE_DEVICE_LOCAL, "cube vertex buffer" );
        s_cubeIndexBuffer = r_globals.device.NewBuffer( sizeof( indices ), indices, BUFFER_TYPE_INDEX, MEMORY_TYPE_DEVICE_LOCAL, "cube index buffer" );
    }

    // DESCRIPTOR SETS
    {
        std::vector< VkDescriptorImageInfo > imgDescriptors;
        std::vector< VkDescriptorBufferInfo > bufferDescriptors;
        std::vector< VkWriteDescriptorSet > writeDescriptorSets;

        postProcessDescriptorSet = r_globals.descriptorPool.NewDescriptorSet( postProcessPipeline.GetResourceLayout()->sets[0] );
        imgDescriptors      = { DescriptorImageInfo( s_renderGraph.GetPhysicalResource( "litOutput" )->texture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ), };
        writeDescriptorSets = { WriteDescriptorSet( postProcessDescriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &imgDescriptors[0] ), };
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
        skyboxDescriptorSet = r_globals.descriptorPool.NewDescriptorSet( skyboxPipeline.GetResourceLayout()->sets[0] );
    }

    return true;
}


void Shutdown()
{
    r_globals.device.WaitForIdle();

    s_renderGraph.Free();
    depthOnlyPipeline.Free();
    litPipeline.Free();
    skyboxPipeline.Free();
    postProcessPipeline.Free();
    
    sceneGlobals.UnMap();
    sceneGlobals.Free();
    s_gpuPointLights.UnMap();
    s_gpuPointLights.Free();
    s_cubeIndexBuffer.Free();
    s_cubeVertexBuffer.Free();

    R_Shutdown();
}


GPU::MaterialData CPUMaterialToGPU( Material* material )
{
    GPU::MaterialData gpuMaterial;
    gpuMaterial.albedoTint = glm::vec4( material->albedoTint, 1 );
    gpuMaterial.metalnessTint = material->metalnessTint;
    gpuMaterial.roughnessTint = material->roughnessTint;
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

    Gfx::TextureManager::UpdateDescriptors( bindlessTexturesDescriptorSet );

    r_globals.swapChainImageIndex = r_globals.swapchain.AcquireNextImage( r_globals.presentCompleteSemaphore );

    auto& cmdBuf = r_globals.graphicsCommandBuffer;
    cmdBuf.BeginRecording();
    PG_PROFILE_GPU_RESET( cmdBuf );
    PG_PROFILE_GPU_START( cmdBuf, "Frame" );

    UpdateGlobalAndLightBuffers( scene );
    if ( skyboxTexture != &scene->skybox->gpuTexture )
    {
        skyboxTexture = &scene->skybox->gpuTexture;
        std::vector< VkDescriptorImageInfo > imgDescriptors;
        std::vector< VkWriteDescriptorSet > writeDescriptorSets;

        imgDescriptors      = { DescriptorImageInfo( *skyboxTexture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ), };
        writeDescriptorSets = { WriteDescriptorSet( skyboxDescriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &imgDescriptors[0] ), };
        r_globals.device.UpdateDescriptorSets( static_cast< uint32_t >( writeDescriptorSets.size() ), writeDescriptorSets.data() );
    }

    s_renderGraph.Render( scene, &cmdBuf );

    PG_PROFILE_GPU_END( cmdBuf, "Frame" );
    cmdBuf.EndRecording();
    r_globals.device.SubmitRenderCommands( 1, &cmdBuf );
    r_globals.device.SubmitFrame( r_globals.swapChainImageIndex );
    PG_PROFILE_GPU_GET_RESULTS();
}


static void RenderFunc_DepthPass( RenderTask* task, Scene* scene, CommandBuffer* cmdBuf )
{
    cmdBuf->BindPipeline( &depthOnlyPipeline );
    cmdBuf->BindDescriptorSet( sceneGlobalDescriptorSet, PG_SCENE_GLOBALS_BUFFER_SET );
    cmdBuf->SetViewport( SceneSizedViewport() );
    cmdBuf->SetScissor( SceneSizedScissor() );
    glm::mat4 VP = scene->camera.GetVP();
    scene->registry.view< ModelRenderer, Transform >().each( [&]( ModelRenderer& renderer, Transform& transform )
    {
        const auto& model = renderer.model;
        auto M = transform.GetModelMatrix();
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
    cmdBuf->BindPipeline( &litPipeline );
    cmdBuf->SetViewport( SceneSizedViewport() );
    cmdBuf->SetScissor( SceneSizedScissor() );
    cmdBuf->BindDescriptorSet( sceneGlobalDescriptorSet, PG_SCENE_GLOBALS_BUFFER_SET );
    cmdBuf->BindDescriptorSet( bindlessTexturesDescriptorSet, PG_BINDLESS_TEXTURE_SET );
    cmdBuf->BindDescriptorSet( lightsDescriptorSet, PG_LIGHTS_SET );
    scene->registry.view< ModelRenderer, Transform >().each( [&]( ModelRenderer& modelRenderer, Transform& transform )
    {
        const auto& model = modelRenderer.model;
        auto M = transform.GetModelMatrix();
        auto N = glm::transpose( glm::inverse( M ) );
        GPU::PerObjectData perObjData{ M, N };
        cmdBuf->PushConstants( 0, sizeof( GPU::PerObjectData ), &perObjData );

        cmdBuf->BindVertexBuffer( model->vertexBuffer, model->gpuPositionOffset, 0 );
        cmdBuf->BindVertexBuffer( model->vertexBuffer, model->gpuNormalOffset, 1 );
        cmdBuf->BindVertexBuffer( model->vertexBuffer, model->gpuTexCoordOffset, 2 );
        cmdBuf->BindIndexBuffer( model->indexBuffer );

        for ( size_t i = 0; i < model->meshes.size(); ++i )
        {
            const Mesh& mesh   = modelRenderer.model->meshes[i];
            Material* material = modelRenderer.materials[i];
            GPU::MaterialData gpuMaterial = CPUMaterialToGPU( material );
            cmdBuf->PushConstants( 128, sizeof( gpuMaterial ), &gpuMaterial );
            PG_DEBUG_MARKER_INSERT_CMDBUF( (*cmdBuf), "Draw \"" + model->name + "\" : \"" + mesh.name + "\"", glm::vec4( 0 ) );
            cmdBuf->DrawIndexed( mesh.startIndex, mesh.numIndices, mesh.startVertex );
        }
    });
}


static void RenderFunc_SkyboxPass( RenderTask* task, Scene* scene, CommandBuffer* cmdBuf )
{
    cmdBuf->BindPipeline( &skyboxPipeline );
    cmdBuf->SetViewport( SceneSizedViewport() );
    cmdBuf->SetScissor( SceneSizedScissor() );
    cmdBuf->BindDescriptorSet( skyboxDescriptorSet, 0 );
    
    cmdBuf->BindVertexBuffer( s_cubeVertexBuffer );
    cmdBuf->BindIndexBuffer( s_cubeIndexBuffer, IndexType::UNSIGNED_SHORT );
    glm::mat4 cubeVP = scene->camera.GetP() * glm::mat4( glm::mat3( scene->camera.GetV() ) );
    cmdBuf->PushConstants( 0, sizeof( cubeVP ), &cubeVP );
    cmdBuf->DrawIndexed( 0, 36 );
}


static void RenderFunc_PostProcessPass( RenderTask*  task, Scene* scene, CommandBuffer* cmdBuf )
{
    cmdBuf->BindPipeline( &postProcessPipeline );
    cmdBuf->SetViewport( DisplaySizedViewport() );
    cmdBuf->SetScissor( DisplaySizedScissor() );
    cmdBuf->BindDescriptorSet( postProcessDescriptorSet, 0 );
    cmdBuf->Draw( 0, 6 );
}


static bool InitRenderGraph( int width, int height )
{
    RenderTaskBuilder* task;
    RenderGraphBuilder builder;

    task = builder.AddTask( "depth_prepass" );
    task->AddDepthOutput( "depth", PixelFormat::DEPTH_32_FLOAT, SIZE_SCENE(), SIZE_SCENE(), 1 );
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
    builder.SetBackbufferResource( "finalOutput" );
    
    RenderGraphCompileInfo compileInfo;
    compileInfo.sceneWidth = width;
    compileInfo.sceneHeight = height;
    compileInfo.displayWidth = r_globals.swapchain.GetWidth();
    compileInfo.displayHeight = r_globals.swapchain.GetHeight();
    if ( !s_renderGraph.Compile( builder, compileInfo ) )
    {
        LOG_ERR( "Failed to compile task graph" );
        return false;
    }

    s_renderGraph.Print();
    
    return true;
}


} // namespace RenderSystem
} // namespace PG
