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
    // 1024×576
    EngineInitInfo engineInitConfig;
    engineInitConfig.sceneWidth   = 1920;
    engineInitConfig.sceneHeight  = 1080;
    engineInitConfig.windowWidth  = 1920;
    engineInitConfig.windowHeight = 1080;
    if ( !EngineInitialize( engineInitConfig ) )
    {
        LOG_ERR( "Failed to initialize the engine" );
        return 0;
    }

#if USING( LOAD_SCENE_DIRECTLY )
    std::string sceneName = argc > 1 ? argv[1] : "";
    // Scene* scene          = Scene::Load( "assets/basic.json" );
    Scene* scene = Scene::Load( PG_ASSET_DIR "scenes/" + sceneName + ".json" );
    if ( !scene )
    {
        EngineShutdown();
        return 0;
    }
#else  // #if USING( LOAD_SCENE_DIRECTLY )
    UI::BootMainMenu();
#endif // #else // #if USING( LOAD_SCENE_DIRECTLY )

    Window* window = GetMainWindow();
    window->SetRelativeMouse( USING( LOAD_SCENE_DIRECTLY ) );
    // window->SetRelativeMouse( false );

    Time::Reset();

    while ( !eg.shutdown )
    {
        window->StartFrame();
        ProcessPendingConsoleCommands();
        AssetManager::ProcessPendingLiveUpdates();
        Input::PollEvents();

        if ( eg.resizeRequested )
        {
            RenderSystem::Resize( window->FramebufferWidth(), window->FramebufferHeight() );
        }

        if ( auto primaryScenePtr = GetPrimaryScene() )
        {
            primaryScenePtr->Update();
        }
        UI::Update();

        RenderSystem::Render();

        // std::this_thread::sleep_for( (std::chrono::milliseconds)10 );

        window->EndFrame();
    }

    EngineShutdown();

    return 0;
}
