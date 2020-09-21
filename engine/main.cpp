#include "engine_init.hpp"
#include "core/input.hpp"
#include "core/time.hpp"
#include "core/window.hpp"
#include "asset/asset_manager.hpp"
#include "renderer/render_system.hpp"
#include <iostream>
#include <thread>

using namespace PG;

bool g_paused = false;

int main( int argc, char* argv[] )
{
    // if ( argc != 2 )
    // {
    //     std::cout << "Usage: ./example [path to scene file]" << std::endl;
    //     return 0;
    // }

    if ( !EngineInitialize() )
    {
        std::cout << "Failed to initialize the engine" << std::endl;
        return 0;
    }

    {
        Window* window = GetMainWindow();
        window->SetRelativeMouse( true );
    
        //Scene* scene = Scene::Load( argv[1] );
        //if ( !scene )
        //{
        //    LOG_ERR( "Could not load scene '", argv[1], "'" );
        //    PG::EngineQuit();
        //    return 0;
        //}
        // scene->Start();

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

            //RenderSystem::Render( scene );
            RenderSystem::Render( nullptr );

            window->EndFrame();
        }

        //Gfx::g_renderState.device.WaitForIdle();
        //delete scene;
    }

    EngineShutdown();

    return 0;
}
