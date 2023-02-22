#include "asset/asset_manager.hpp"
#include "asset/types/model.hpp"
#include "asset/types/shader.hpp"
#include "core/init.hpp"
#include "core/input.hpp"
#include "core/scene.hpp"
#include "core/time.hpp"
#include "core/window.hpp"
#include "image.hpp"
#include "renderer/graphics_api.hpp"
#include "renderer/render_system.hpp"
#include "renderer/rendergraph/r_rendergraph.hpp"
#include "shared/logger.hpp"
#include "shared/math.hpp"
#include "ui/ui_system.hpp"


using namespace PG;
using namespace Gfx;

bool g_paused = false;

int main( int argc, char* argv[] )
{
    EngineInitInfo engineInitConfig;
	if ( !EngineInitialize( engineInitConfig ) )
    {
        LOG_ERR( "Failed to initialize the engine" );
        return 1;
    }

    AssetManager::LoadFastFile( "mkdd" );
    UI::BootMainMenu();

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
        UI::Update();

        RenderSystem::Render();

        window->EndFrame();
    }

    EngineShutdown();

    return 0;
}
