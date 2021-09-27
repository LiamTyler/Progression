#include "engine_init.hpp"
#include "core/input.hpp"
#include "core/time.hpp"
#include "core/window.hpp"
#include "asset/asset_manager.hpp"
#include "asset/types/shader.hpp"
#include "renderer/graphics_api.hpp"
#include "renderer/render_system.hpp"
#include "utils/logger.hpp"
#include <iostream>
#include <thread>

using namespace PG;
using namespace Gfx;

bool g_paused = false;

int main( int argc, char* argv[] )
{
	EngineInitInfo engineInitConfig;
	engineInitConfig.headless = true; // no window
	if ( !EngineInitialize( engineInitConfig ) )
    {
        LOG_ERR( "Failed to initialize the engine\n" );
        return 0;
    }

	const int BUFFER_SIZE = 64;
	float cpu_a[BUFFER_SIZE];
	float cpu_b[BUFFER_SIZE];
	for ( int i = 0; i < BUFFER_SIZE; ++i )
	{
		cpu_a[i] = (float) i;
		cpu_b[i] = (float) i + 2; 
	}

    Device& device = r_globals.device;
	Buffer gpu_a = device.NewBuffer( BUFFER_SIZE * sizeof( float ), cpu_a, BUFFER_TYPE_STORAGE, MEMORY_TYPE_HOST_VISIBLE, "ssbo_a" );
	Buffer gpu_b = device.NewBuffer( BUFFER_SIZE * sizeof( float ), cpu_b, BUFFER_TYPE_STORAGE, MEMORY_TYPE_HOST_VISIBLE, "ssbo_b" );
	Buffer gpu_c = device.NewBuffer( BUFFER_SIZE * sizeof( float ), BUFFER_TYPE_STORAGE, MEMORY_TYPE_HOST_VISIBLE, "ssbo_c" );
	
	ShaderCreateInfo shaderInfo = { "vector add", PG_ROOT_DIR "old_examples/vector_add.comp", ShaderStage::COMPUTE };
	Shader compShader;
	if ( !Shader_Load( &compShader, shaderInfo ) )
    {
        LOG_ERR( "Could not load shader '%s'\n", shaderInfo.filename.c_str() );
    }
    Pipeline computePipeline = device.NewComputePipeline( &compShader );

    DescriptorSet descriptorSet = r_globals.descriptorPool.NewDescriptorSet( computePipeline.GetResourceLayout()->sets[0] );
    std::vector< VkDescriptorBufferInfo > bufferDescriptors =
    {
        DescriptorBufferInfo( gpu_a ),
        DescriptorBufferInfo( gpu_b ),
        DescriptorBufferInfo( gpu_c ),
    };
    std::vector< VkWriteDescriptorSet > writeDescriptorSets =
    {
        WriteDescriptorSet( descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, &bufferDescriptors[0] ),
        WriteDescriptorSet( descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, &bufferDescriptors[1] ),
        WriteDescriptorSet( descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2, &bufferDescriptors[2] ),
    };
    device.UpdateDescriptorSets( static_cast< uint32_t >( writeDescriptorSets.size() ), writeDescriptorSets.data() );

    CommandBuffer cmdBuf = r_globals.commandPools[GFX_CMD_POOL_COMPUTE].NewCommandBuffer();
    cmdBuf.BeginRecording();
    cmdBuf.BindPipeline( computePipeline );
    cmdBuf.BindDescriptorSets( 1, &descriptorSet, computePipeline );
    cmdBuf.Dispatch( 64, 1, 1 );
    cmdBuf.EndRecording();
    device.SubmitComputeCommand( cmdBuf );
    device.WaitForIdle();

    float cpu_c[BUFFER_SIZE] = { 0 };
    gpu_c.ReadToCpu( cpu_c );
    int correct = 0;
    for ( int i = 0; i < BUFFER_SIZE; ++i )
	{
		if ( cpu_c[i] != cpu_a[i] + cpu_b[i] )
        {
            LOG_ERR( "Element %d not correct! (%f != %f)\n", i, cpu_c[i], cpu_a[i] + cpu_b[i] );
        }
        else
        {
            correct++;
        }
	}

    if ( correct == BUFFER_SIZE )
    {
        LOG( "ALL CORRECT!\n" );
    }


    gpu_a.Free();
    gpu_b.Free();
    gpu_c.Free();
    computePipeline.Free();
    compShader.Free();
    EngineShutdown();

    return 0;
}
