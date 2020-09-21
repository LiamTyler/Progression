#include "renderer/render_system.hpp"
#include "renderer/graphics_api.hpp"
#include "renderer/vulkan.hpp"
#include "renderer/render_system.hpp"
#include "renderer/r_globals.hpp"
#include "asset/asset_manager.hpp"
#include "asset/types/shader.hpp"
#include "core/assert.hpp"
#include "core/window.hpp"
#include "utils/logger.hpp"

using namespace PG;
using namespace Gfx;

static Window* s_window;

R_Globals PG::Gfx::r_globals;

Pipeline pipeline;


namespace PG
{
namespace RenderSystem
{

bool Init()
{
    s_window = GetMainWindow();
    if ( !VulkanInit() )
    {
        LOG_ERR( "Could not init Vulkan\n" );
        return false;
    }

    if ( !Profile::Init() )
    {
        LOG_ERR( "Could not initialize gpu profiler" );
        return false;
    }

    AssetManager::LoadFastFile( "basic" );

    auto vertShader = AssetManager::Get< Shader >( "basicVert" );
    auto fragShader = AssetManager::Get< Shader >( "basicFrag" );
    PG_ASSERT( vertShader && fragShader );

    PipelineDescriptor pipelineDesc;
    pipelineDesc.renderPass             = &g_renderState.renderPass;
    pipelineDesc.descriptorSetLayouts   = {};
    pipelineDesc.vertexDescriptor       = VertexInputDescriptor::Create( 0, nullptr, 0, nullptr );
    pipelineDesc.rasterizerInfo.winding = WindingOrder::CLOCKWISE;
    pipelineDesc.viewport               = FullScreenViewport();
    pipelineDesc.scissor                = FullScreenScissor();
    pipelineDesc.shaders[0]             = vertShader;
    pipelineDesc.shaders[1]             = fragShader;

    pipeline = r_globals.device->NewPipeline( pipelineDesc, "rigid Model" );
    if ( !pipeline )
    {
        LOG_ERR( "Could not create pipeline" );
        return false;
    }

    return true;
}


void Shutdown()
{
    g_renderState.device.WaitForIdle();
    Profile::Shutdown();
    pipeline.Free();
    VulkanShutdown();
}


void Render( Scene* scene )
{
    //PG_ASSERT( scene != nullptr );
    auto swapChainImageIndex = g_renderState.swapChain.AcquireNextImage( g_renderState.presentCompleteSemaphore );

    auto& cmdBuf = g_renderState.graphicsCommandBuffer;
    cmdBuf.BeginRecording();
    PG_PROFILE_GPU_RESET( cmdBuf );

    PG_PROFILE_GPU_START( cmdBuf, "Frame" );

    cmdBuf.BeginRenderPass( g_renderState.renderPass, g_renderState.swapChainFramebuffers[swapChainImageIndex], g_renderState.swapChain.extent );

    cmdBuf.BindRenderPipeline( pipeline );
    PG_DEBUG_MARKER_INSERT( cmdBuf, "Draw full-screen quad", glm::vec4( 0 ) );

    cmdBuf.Draw( 0, 3 );

    cmdBuf.EndRenderPass();

    PG_PROFILE_GPU_END( cmdBuf, "Frame" );

    cmdBuf.EndRecording();
    g_renderState.device.SubmitRenderCommands( 1, &cmdBuf );
    g_renderState.device.SubmitFrame( swapChainImageIndex );

    PG_PROFILE_GPU_GET_RESULTS();
} 


} // namespace RenderSystem
} // namespace PG
