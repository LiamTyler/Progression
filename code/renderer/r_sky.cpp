#include "r_sky.hpp"
#include "asset/types/gfx_image.hpp"
#include "c_shared/sky.h"
#include "core/scene.hpp"
#include "r_pipeline_manager.hpp"

namespace PG::Gfx::Sky
{

static Buffer s_cubeVertexBuffer;
static Buffer s_cubeIndexBuffer;
static Pipeline* s_pipeline;

void Init()
{
    // clang-format off
    vec3 verts[] =
    {
        vec3( -1, -1, -1 ),
        vec3( -1,  1, -1 ),
        vec3(  1, -1, -1 ),
        vec3(  1,  1, -1 ),
        vec3( -1, -1,  1 ),
        vec3( -1,  1,  1 ),
        vec3(  1, -1,  1 ),
        vec3(  1,  1,  1 ),
    };
    uint16_t indices[] =
    {
        0, 1, 3, // -Z face
        0, 3, 2,

        6, 7, 5, // +Z face
        6, 5, 4,

        2, 3, 7, // +X face
        2, 7, 6,
        
        4, 5, 1, // -X face
        4, 1, 0,

        4, 0, 2, // -Y face
        4, 2, 6,
        
        1, 5, 7, // +Y face
        1, 7, 3,
    };
    // clang-format on

    BufferCreateInfo createInfo = {};
    createInfo.size             = sizeof( verts );
    createInfo.initalData       = verts;
    createInfo.bufferUsage      = BufferUsage::TRANSFER_DST | BufferUsage::STORAGE | BufferUsage::DEVICE_ADDRESS;
    s_cubeVertexBuffer          = rg.device.NewBuffer( createInfo, "sky_cubeVB" );

    createInfo.size        = sizeof( indices );
    createInfo.initalData  = indices;
    createInfo.bufferUsage = BufferUsage::TRANSFER_DST | BufferUsage::INDEX;
    s_cubeIndexBuffer      = rg.device.NewBuffer( createInfo, "sky_cubeIB" );

    s_pipeline = PipelineManager::GetPipeline( "sky", USING( DEVELOPMENT_BUILD ) );
}

void Shutdown()
{
    s_cubeVertexBuffer.Free();
    s_cubeIndexBuffer.Free();
}

void DrawSkyTask( GraphicsTask* task, TGExecuteData* data )
{
    CommandBuffer& cmdBuf = *data->cmdBuf;

    cmdBuf.BindPipeline( s_pipeline );
    cmdBuf.BindGlobalDescriptors();
    cmdBuf.SetViewport( SceneSizedViewport() );
    cmdBuf.SetScissor( SceneSizedScissor() );

    GpuData::SkyDrawData pushConstants;
    pushConstants.VP              = data->scene->camera.GetP() * mat4( mat3( data->scene->camera.GetV() ) );
    pushConstants.vertexBuffer    = s_cubeVertexBuffer.GetDeviceAddress();
    pushConstants.skyTextureIndex = data->scene->skybox->gpuTexture.GetBindlessIndex();
    cmdBuf.PushConstants( pushConstants );

    cmdBuf.BindIndexBuffer( s_cubeIndexBuffer, IndexType::UNSIGNED_SHORT );
    cmdBuf.DrawIndexed( 0, 36 );
}

void AddTask( TaskGraphBuilder& builder, TGBTextureRef litOutput, TGBTextureRef sceneDepth )
{
    GraphicsTaskBuilder* task = builder.AddGraphicsTask( "Sky" );
    task->AddColorAttachment( litOutput );
    task->AddDepthAttachment( sceneDepth );
    task->SetFunction( DrawSkyTask );
}

} // namespace PG::Gfx::Sky
