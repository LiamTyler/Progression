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
#include "core/math.hpp"
#include "asset/image.hpp"
#include "renderer/rendergraph/r_rendergraph.hpp"

using namespace PG;
using namespace Gfx;

bool g_paused = false;


int main( int argc, char* argv[] )
{
    //EngineInitInfo engineInitConfig;
	//if ( !EngineInitialize( engineInitConfig ) )
    //{
    //    LOG_ERR( "Failed to initialize the engine" );
    //    return 1;
    //}

    RenderTaskBuilder* task;
    RenderGraphBuilder builder;

    task = builder.AddTask( "depth_prepass" );
    task->AddDepthOutput( "depth", PixelFormat::DEPTH_32_FLOAT, SIZE_SCENE(), SIZE_SCENE(), 1 );

    task = builder.AddTask( "lighting" );
    task->AddColorOutput( "litOutput", PixelFormat::R16_G16_B16_A16_FLOAT, SIZE_SCENE(), SIZE_SCENE(), 1, 1, 1, glm::vec4( 0 ) );
    task->AddDepthOutput( "depth" );

    task = builder.AddTask( "skybox" );
    task->AddColorOutput( "litOutput" );
    task->AddDepthOutput( "depth" );
    
    task = builder.AddTask( "postProcessing" );
    task->AddTextureInput( "litOutput" );
    task->AddColorOutput( "finalOutput", PixelFormat::R8_G8_B8_A8_UNORM, SIZE_DISPLAY(), SIZE_DISPLAY(), 1, 1, 1 );
    
    RenderGraph graph;
    RenderGraphCompileInfo compileInfo;
    compileInfo.sceneWidth = 1280;
    compileInfo.sceneHeight = 720;
    compileInfo.displayWidth = 1980;
    compileInfo.displayHeight = 1080;
    if ( !graph.Compile( builder, compileInfo ) )
    {
        printf( "Failed to compile task graph\n" );
        return false;
    }
    
    graph.PrintTaskGraph();

    //EngineShutdown();
    return 0;

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
    */
}
