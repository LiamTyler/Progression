#include "renderer/render_system.hpp"
#include "renderer/graphics_api.hpp"
#include "renderer/render_system.hpp"
#include "renderer/r_globals.hpp"
#include "renderer/r_init.hpp"
#include "renderer/r_renderpasses.hpp"
#include "asset/asset_manager.hpp"
#include "glm/glm.hpp"
#include "shaders/c_shared/structs.h"
#include "core/assert.hpp"
#include "core/scene.hpp"
#include "core/window.hpp"
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
Buffer sceneGlobals;


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

    AssetManager::LoadFastFile( "gfx_required" );

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
    pipelineDesc.depthInfo.compareFunc  = CompareFunction::EQUAL;
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

    postProcessDescriptorSet = r_globals.descriptorPool.NewDescriptorSet( postProcessPipeline.GetResourceLayout()->sets[0] );
    std::vector< VkDescriptorImageInfo > imgDescriptors =
    {
        DescriptorImageInfo( r_globals.colorTex, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ),
    };
    std::vector< VkWriteDescriptorSet > writeDescriptorSets =
    {
        WriteDescriptorSet( postProcessDescriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &imgDescriptors[0] ),
    };
    r_globals.device.UpdateDescriptorSets( static_cast< uint32_t >( writeDescriptorSets.size() ), writeDescriptorSets.data() );

    sceneGlobals = r_globals.device.NewBuffer( sizeof( SceneGlobals ), BUFFER_TYPE_UNIFORM, MEMORY_TYPE_HOST_VISIBLE, "scene globals ubo" );
    sceneGlobals.Map();
    
    sceneGlobalDescriptorSet = r_globals.descriptorPool.NewDescriptorSet( depthOnlyPipeline.GetResourceLayout()->sets[0] );
    std::vector< VkDescriptorBufferInfo > bufferDescriptors =
    {
        DescriptorBufferInfo( sceneGlobals ),
    };
    writeDescriptorSets =
    {
        WriteDescriptorSet( sceneGlobalDescriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &bufferDescriptors[0] ),
    };
    r_globals.device.UpdateDescriptorSets( static_cast< uint32_t >( writeDescriptorSets.size() ), writeDescriptorSets.data() );


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

    R_Shutdown();
}


void Render( Scene* scene )
{
    //PG_ASSERT( scene != nullptr );
    auto swapChainImageIndex = r_globals.swapchain.AcquireNextImage( r_globals.presentCompleteSemaphore );

    auto& cmdBuf = r_globals.graphicsCommandBuffer;
    cmdBuf.BeginRecording();
    PG_PROFILE_GPU_RESET( cmdBuf );

    PG_PROFILE_GPU_START( cmdBuf, "Frame" );

    Model* model = AssetManager::Get< Model >( "chalet" );

    {
        SceneGlobals globalData;
        globalData.V      = scene->camera.GetV();
        globalData.P      = scene->camera.GetP();
        globalData.VP     = scene->camera.GetVP();
        globalData.invVP  = glm::inverse( scene->camera.GetVP() );
        globalData.cameraPos = glm::vec4( scene->camera.position, 1 );

        memcpy( sceneGlobals.MappedPtr(), &globalData, sizeof( SceneGlobals ) );
        sceneGlobals.FlushCpuWrites();
    }

    // DEPTH
    cmdBuf.BeginRenderPass( GetRenderPass( GFX_RENDER_PASS_DEPTH_PREPASS ), *GetFramebuffer( GFX_RENDER_PASS_DEPTH_PREPASS ) );
    cmdBuf.BindPipeline( depthOnlyPipeline );
    cmdBuf.BindDescriptorSets( 1, &sceneGlobalDescriptorSet, depthOnlyPipeline );
    cmdBuf.SetViewport( FullScreenViewport() );
    cmdBuf.SetScissor( FullScreenScissor() );
    // cmdBuf.BindVertexBuffer( vertexBuffer, 0, 0 );
    // cmdBuf.BindIndexBuffer( indexBuffer, IndexType::UNSIGNED_INT );
    // cmdBuf.Draw( 0, 3 );
    // cmdBuf.EndRenderPass();
    cmdBuf.BindVertexBuffer( model->vertexBuffer, model->gpuPositionOffset, 0 );
    cmdBuf.BindIndexBuffer( model->indexBuffer );
    for ( uint32_t meshIdx = 0; meshIdx < model->meshes.size(); ++meshIdx )
    {
        const Mesh& mesh = model->meshes[meshIdx];
        cmdBuf.DrawIndexed( mesh.startIndex, mesh.numIndices, mesh.startVertex );
    }
    cmdBuf.EndRenderPass();

    // LIT
    cmdBuf.BeginRenderPass( GetRenderPass( GFX_RENDER_PASS_LIT ), *GetFramebuffer( GFX_RENDER_PASS_LIT ) );
    cmdBuf.BindPipeline( litPipeline );
    cmdBuf.SetViewport( FullScreenViewport() );
    cmdBuf.SetScissor( FullScreenScissor() );
    cmdBuf.BindVertexBuffer( model->vertexBuffer, model->gpuPositionOffset, 0 );
    cmdBuf.BindVertexBuffer( model->vertexBuffer, model->gpuNormalOffset, 1 );
    cmdBuf.BindVertexBuffer( model->vertexBuffer, model->gpuTexCoordOffset, 2 );
    cmdBuf.BindIndexBuffer( model->indexBuffer );
    for ( uint32_t meshIdx = 0; meshIdx < model->meshes.size(); ++meshIdx )
    {
        const Mesh& mesh = model->meshes[meshIdx];
        cmdBuf.DrawIndexed( mesh.startIndex, mesh.numIndices, mesh.startVertex );
    }
    cmdBuf.EndRenderPass();
    
    // POST
    cmdBuf.BeginRenderPass( GetRenderPass( GFX_RENDER_PASS_POST_PROCESS ), r_globals.swapchainFramebuffers[swapChainImageIndex] );
    cmdBuf.BindPipeline( postProcessPipeline );
    cmdBuf.SetViewport( FullScreenViewport() );
    cmdBuf.SetScissor( FullScreenScissor() );
    cmdBuf.BindDescriptorSets( 1, &postProcessDescriptorSet, postProcessPipeline );
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
