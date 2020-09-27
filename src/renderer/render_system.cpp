#include "renderer/render_system.hpp"
#include "renderer/graphics_api.hpp"
#include "renderer/render_system.hpp"
#include "renderer/r_globals.hpp"
#include "renderer/r_init.hpp"
#include "asset/asset_manager.hpp"
#include "asset/types/shader.hpp"
#include "core/assert.hpp"
#include "core/window.hpp"
#include "utils/logger.hpp"
#include "glm/glm.hpp"

using namespace PG;
using namespace Gfx;

static Window* s_window;

R_Globals PG::Gfx::r_globals;

Pipeline pipeline;
Buffer vertexBuffer;
Buffer indexBuffer;


namespace PG
{
namespace RenderSystem
{

bool Init()
{
    s_window = GetMainWindow();
    r_globals = {};

    if ( !R_Init( s_window->Width(), s_window->Height() ) )
    {
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

    VertexBindingDescriptor bindingDescs[] =
    {
        VertexBindingDescriptor( 0, sizeof( glm::vec3 ) ),
    };

    VertexAttributeDescriptor attribDescs[] =
    {
        VertexAttributeDescriptor( 0, 0, BufferDataType::FLOAT3, 0 ),
    };

    PipelineDescriptor pipelineDesc;
    pipelineDesc.renderPass             = &r_globals.renderPass;
    pipelineDesc.descriptorSetLayouts   = {};
    pipelineDesc.vertexDescriptor       = VertexInputDescriptor::Create( 1, bindingDescs, 1, attribDescs );
    pipelineDesc.rasterizerInfo.winding = WindingOrder::COUNTER_CLOCKWISE;
    pipelineDesc.viewport               = FullScreenViewport();
    pipelineDesc.scissor                = FullScreenScissor();
    pipelineDesc.shaders[0]             = vertShader;
    pipelineDesc.shaders[1]             = fragShader;

    pipeline = r_globals.device.NewPipeline( pipelineDesc, "rigid Model" );
    if ( !pipeline )
    {
        LOG_ERR( "Could not create pipeline" );
        return false;
    }

    glm::vec3 verts[] =
    {
        glm::vec3( -0.5, 0.5, 0 ),
        glm::vec3( 0.5,  0.5, 0 ),
        glm::vec3( 0.0, -0.5, 0 ),
    };
    uint32_t indices[] =
    {
        0, 1, 2
    };
    vertexBuffer = r_globals.device.NewBuffer( sizeof( verts ), verts, BUFFER_TYPE_VERTEX, MEMORY_TYPE_DEVICE_LOCAL, "vertex" );
    indexBuffer  = r_globals.device.NewBuffer( sizeof( indices ), indices, BUFFER_TYPE_INDEX, MEMORY_TYPE_DEVICE_LOCAL, "index" );

    return true;
}


void Shutdown()
{
    r_globals.device.WaitForIdle();
    
    pipeline.Free();
    vertexBuffer.Free();
    indexBuffer.Free();

    Profile::Shutdown();
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

    cmdBuf.BeginRenderPass( r_globals.renderPass, r_globals.swapchainFramebuffers[swapChainImageIndex] );

    cmdBuf.BindRenderPipeline( pipeline );
    PG_DEBUG_MARKER_INSERT( cmdBuf, "Draw full-screen quad", glm::vec4( 0 ) );
    cmdBuf.BindVertexBuffer( vertexBuffer, 0, 0 );
    cmdBuf.BindIndexBuffer(  indexBuffer, IndexType::UNSIGNED_INT );

    cmdBuf.Draw( 0, 3 );

    cmdBuf.EndRenderPass();

    PG_PROFILE_GPU_END( cmdBuf, "Frame" );

    cmdBuf.EndRecording();
    r_globals.device.SubmitRenderCommands( 1, &cmdBuf );
    r_globals.device.SubmitFrame( swapChainImageIndex );

    PG_PROFILE_GPU_GET_RESULTS();
} 


} // namespace RenderSystem
} // namespace PG
