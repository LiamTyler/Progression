#include "renderer/render_system.hpp"
#include "renderer/graphics_api.hpp"
#include "renderer/render_system.hpp"
#include "renderer/r_globals.hpp"
#include "renderer/r_init.hpp"
#include "renderer/r_renderpasses.hpp"
#include "asset/asset_manager.hpp"
#include "asset/types/shader.hpp"
#include "core/assert.hpp"
#include "core/window.hpp"
#include "utils/logger.hpp"
#include "glm/glm.hpp"
#include <unordered_map>

using namespace PG;
using namespace Gfx;

static Window* s_window;

R_Globals PG::Gfx::r_globals;

Pipeline depthOnlyPipeline;
Pipeline litPipeline;
Pipeline skyboxPipeline;
Pipeline postProcessPipeline;

DescriptorSet descriptorSet;

Buffer vertexBuffer;
Buffer indexBuffer;


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
    };

    VertexAttributeDescriptor attribDescs[2] =
    {
        VertexAttributeDescriptor( 0, 0, BufferDataType::FLOAT3, 0 ),
        VertexAttributeDescriptor( 1, 0, BufferDataType::FLOAT3, 3 * sizeof( glm::vec3 ) ),
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
    pipelineDesc.vertexDescriptor       = VertexInputDescriptor::Create( 1, bindingDescs, 2, attribDescs );
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

    glm::vec3 verts[] =
    {
        glm::vec3( -0.5, 0.5, 0 ),
        glm::vec3( 0.5,  0.5, 0 ),
        glm::vec3( 0.0, -0.5, 0 ),
        glm::vec3( 1, 0, 0 ),
        glm::vec3( 0, 1, 0 ),
        glm::vec3( 0, 0, 1 ),
    };
    uint32_t indices[] =
    {
        0, 1, 2
    };
    vertexBuffer = r_globals.device.NewBuffer( sizeof( verts ), verts, BUFFER_TYPE_VERTEX, MEMORY_TYPE_DEVICE_LOCAL, "vertex" );
    indexBuffer  = r_globals.device.NewBuffer( sizeof( indices ), indices, BUFFER_TYPE_INDEX, MEMORY_TYPE_DEVICE_LOCAL, "index" );

    descriptorSet = r_globals.descriptorPool.NewDescriptorSet( postProcessPipeline.GetResourceLayout()->sets[0] );
    std::vector< VkDescriptorImageInfo > imgDescriptors =
    {
        DescriptorImageInfo( r_globals.colorTex, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ),
    };
    std::vector< VkWriteDescriptorSet > writeDescriptorSets =
    {
        WriteDescriptorSet( descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &imgDescriptors[0] ),
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
    
    vertexBuffer.Free();
    indexBuffer.Free();

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

    // DEPTH
    cmdBuf.BeginRenderPass( GetRenderPass( GFX_RENDER_PASS_DEPTH_PREPASS ), *GetFramebuffer( GFX_RENDER_PASS_DEPTH_PREPASS ) );
    cmdBuf.BindPipeline( depthOnlyPipeline );
    cmdBuf.SetViewport( FullScreenViewport() );
    cmdBuf.SetScissor( FullScreenScissor() );
    //PG_DEBUG_MARKER_INSERT( cmdBuf, "Draw full-screen quad", glm::vec4( 0 ) );
    cmdBuf.BindVertexBuffer( vertexBuffer, 0, 0 );
    cmdBuf.BindIndexBuffer( indexBuffer, IndexType::UNSIGNED_INT );
    cmdBuf.Draw( 0, 3 );
    cmdBuf.EndRenderPass();

    // LIT
    cmdBuf.BeginRenderPass( GetRenderPass( GFX_RENDER_PASS_LIT ), *GetFramebuffer( GFX_RENDER_PASS_LIT ) );
    cmdBuf.BindPipeline( litPipeline );
    cmdBuf.SetViewport( FullScreenViewport() );
    cmdBuf.SetScissor( FullScreenScissor() );
    cmdBuf.BindVertexBuffer( vertexBuffer, 0, 0 );
    cmdBuf.BindIndexBuffer( indexBuffer, IndexType::UNSIGNED_INT );
    cmdBuf.Draw( 0, 3 );
    cmdBuf.EndRenderPass();

    // POST
    cmdBuf.BeginRenderPass( GetRenderPass( GFX_RENDER_PASS_POST_PROCESS ), r_globals.swapchainFramebuffers[swapChainImageIndex] );
    cmdBuf.BindPipeline( postProcessPipeline );
    cmdBuf.SetViewport( FullScreenViewport() );
    cmdBuf.SetScissor( FullScreenScissor() );
    cmdBuf.BindDescriptorSets( 1, &descriptorSet, postProcessPipeline );
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
