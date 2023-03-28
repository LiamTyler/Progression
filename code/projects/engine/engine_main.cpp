#include "asset/asset_manager.hpp"
#include "asset/types/model.hpp"
#include "asset/types/shader.hpp"
#include "core/console_commands.hpp"
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
    engineInitConfig.windowWidth = 1024;
    engineInitConfig.windowHeight = 768;
	if ( !EngineInitialize( engineInitConfig ) )
    {
        LOG_ERR( "Failed to initialize the engine" );
        return 1;
    }

    UI::BootMainMenu();

    //AssetManager::LoadFastFile( "sponza" );

    if ( false )
    {
        sol::state lua;
        lua.open_libraries( sol::lib::base, sol::lib::math );
        lua.script( R"(
        t = 0
        function Start()
            t = t + 1
        end

        Start()

        print( t )
        )");

        lua["Start"]();
        LOG( "%d", lua.get<int>( "t" ) );

        lua.script( R"(
        t = 0
        function Start()
            t = t + 2
        end

        Start()

        print( t )
        )");

        lua["Start"]();
        LOG( "%d", lua.get<int>( "t" ) );


        EngineShutdown();
        return 0;
    }

    

    Window* window = GetMainWindow();
    window->SetRelativeMouse( false );

    Input::PollEvents();
    Time::Reset();

    while ( !g_engineShutdown )
    {
        window->StartFrame();
        ProcessPendingCommands();
        AssetManager::ProcessPendingLiveUpdates();
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
