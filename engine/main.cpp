#include "engine_init.hpp"
#include "core/input.hpp"
#include "core/scene.hpp"
#include "core/time.hpp"
#include "core/window.hpp"
#include "asset/asset_manager.hpp"
#include "asset/types/model.hpp"
#include "asset/types/shader.hpp"
#include "renderer/graphics_api.hpp"
#include "renderer/render_system.hpp"
#include "utils/logger.hpp"

#include "renderer/taskgraph/r_taskgraph.hpp"

using namespace PG;
using namespace Gfx;

bool g_paused = false;

int main( int argc, char* argv[] )
{
    Logger_Init();
    Logger_AddLogLocation( "stdout", stdout );

    RenderTaskBuilder* task;
    RenderGraph graph;

    //task = graph.AddTask( "depth_prepass" );

    task = graph.AddTask( "gbuffer" );
    task->AddColorOutput( "albedo", PixelFormat::R8_G8_B8_A8_UNORM, SCENE_WIDTH(), SCENE_HEIGHT(), 1, 1, 1, glm::vec4( 0 ) );
    task->AddColorOutput( "normal", PixelFormat::R16_G16_B16_A16_FLOAT, SCENE_WIDTH(), SCENE_HEIGHT(), 1, 1, 1, glm::vec4( 0 ) );
    
    task = graph.AddTask( "lighting" );
    task->AddTextureInput( "albedo" );
    task->AddTextureInput( "normal" );
    task->AddColorOutput( "litOutput", PixelFormat::R16_G16_B16_A16_FLOAT, SCENE_WIDTH(), SCENE_HEIGHT(), 1, 1, 1, glm::vec4( 0 ) );
    
    task = graph.AddTask( "transparency" );
    task->AddColorOutput( "litOutput" );
    
    task = graph.AddTask( "gbuffer2" );
    task->AddColorOutput( "albedo2", PixelFormat::R8_G8_B8_A8_UNORM, SCENE_WIDTH(), SCENE_HEIGHT(), 1, 1, 1, glm::vec4( 0 ) );
    task->AddColorOutput( "normal2", PixelFormat::R16_G16_B16_A16_FLOAT, SCENE_WIDTH(), SCENE_HEIGHT(), 1, 1, 1, glm::vec4( 0 ) );
    
    task = graph.AddTask( "lighting2" );
    task->AddTextureInput( "albedo2" );
    task->AddTextureInput( "normal2" );
    task->AddColorOutput( "litOutput" );
    
    task = graph.AddTask( "transparency2" );
    task->AddColorOutput( "litOutput" );
    
    task = graph.AddTask( "postProcessing" );
    task->AddColorOutput( "BACK_BUFFER" );
    
    if ( !graph.Compile( 1920, 1080 ) )
    {
        printf( "Failed to compile task graph\n" );
        return 1;
    }
    
    graph.PrintTaskGraph();
}

/*
	EngineInitInfo engineInitConfig;
	if ( !EngineInitialize( engineInitConfig ) )
    {
        LOG_ERR( "Failed to initialize the engine" );
        return 1;
    }

    if ( argc < 2 )
    {
        LOG_ERR( "First argument needs to be path to scene file, relative to folder: %s", PG_ASSET_DIR );
        EngineShutdown();
        return 1;
    }

    Scene* scene = Scene::Load( PG_ASSET_DIR + std::string( argv[1] ) );
    if ( !scene )
    {
        return 1;
    }

    {
        Window* window = GetMainWindow();
        window->SetRelativeMouse( true );

        Input::PollEvents();
        Time::Reset();

        while ( !g_engineShutdown )
        {
            window->StartFrame();
            Input::PollEvents();

            if ( Input::GetKeyDown( Key::ESC ) )
            {
                g_engineShutdown = true;
            }

            scene->Update();

            RenderSystem::Render( scene );

            window->EndFrame();
        }

        delete scene;
    }

    EngineShutdown();

    return 0;
}
*/
