#include "asset/types/model.hpp"
#include "asset/types/shader.hpp"
#include "core/console_commands.hpp"
#include "core/engine_globals.hpp"
#include "core/init.hpp"
#include "core/scene.hpp"
#include "image.hpp"
#include "renderer/render_system.hpp"
#include "ui/ui_system.hpp"

using namespace PG;
using namespace Gfx;

bool g_paused = false;

#define LOAD_SCENE_DIRECTLY IN_USE

int main( int argc, char* argv[] )
{
    EngineInitInfo engineInitConfig;
    engineInitConfig.headless     = false;
    float scale                   = 1.0f;
    engineInitConfig.sceneWidth   = static_cast<int>( scale * 1280 );
    engineInitConfig.sceneHeight  = static_cast<int>( scale * 720 );
    engineInitConfig.windowWidth  = static_cast<int>( scale * 1280 );
    engineInitConfig.windowHeight = static_cast<int>( scale * 720 );
    if ( !EngineInitialize( engineInitConfig ) )
    {
        LOG_ERR( "Failed to initialize the engine" );
        return 0;
    }

    // UI::BootMainMenu();

#if USING( LOAD_SCENE_DIRECTLY )
    std::string sceneName = argc > 1 ? argv[1] : "";
    // Scene* scene          = LoadScene( "basic" );
    Scene* scene = LoadScene( sceneName );
    if ( !scene )
    {
        EngineShutdown();
        return 0;
    }
    SetPrimaryScene( scene );
#else  // #if USING( LOAD_SCENE_DIRECTLY )
    Scene* scene = LoadScene( "startup" );
    if ( !scene )
    {
        EngineShutdown();
        return 0;
    }
    SetPrimaryScene( scene );
#endif // #else // #if USING( LOAD_SCENE_DIRECTLY )

    Window* window = GetMainWindow();
    Time::Reset();

    while ( !eg.shutdown )
    {
        window->StartFrame();
        ProcessPendingConsoleCommands();
        AssetManager::ProcessPendingLiveUpdates();
        Input::PollEvents();

        if ( eg.shutdown )
            break;

        if ( eg.resizeRequested )
        {
            RenderSystem::Resize( window->FramebufferWidth(), window->FramebufferHeight() );
        }
        UI::BeginFrame();

        if ( auto primaryScenePtr = GetPrimaryScene() )
        {
            primaryScenePtr->Update();
        }
        UI::EndFrame();
        RenderSystem::Render();

        // std::this_thread::sleep_for( (std::chrono::milliseconds)10 );

        window->EndFrame();
    }

    EngineShutdown();

    return 0;
}
