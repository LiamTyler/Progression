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

using namespace PG;
using namespace Gfx;

bool g_paused = false;

int main( int argc, char* argv[] )
{
	EngineInitInfo engineInitConfig;
	if ( !EngineInitialize( engineInitConfig ) )
    {
        LOG_ERR( "Failed to initialize the engine" );
        return 0;
    }

    Scene* scene = Scene::Load( PG_ASSET_DIR "scenes/singleMeshViewer.json" );
    if ( !scene )
    {
        return 0;
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

            RenderSystem::Render( scene );

            window->EndFrame();
        }

        delete scene;
    }

    EngineShutdown();

    return 0;
}
