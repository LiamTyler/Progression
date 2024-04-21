#include "asset/asset_manager.hpp"
#include "asset/types/model.hpp"
#include "asset/types/shader.hpp"
#include "core/console_commands.hpp"
#include "core/dvars.hpp"
#include "core/init.hpp"
#include "core/input.hpp"
#include "core/scene.hpp"
#include "core/time.hpp"
#include "core/window.hpp"
#include "image.hpp"
#include "renderer/graphics_api.hpp"
#include "renderer/render_system.hpp"
#include "renderer/rendergraph/r_rendergraph2.hpp"
#include "shared/logger.hpp"
#include "ui/ui_system.hpp"


using namespace PG;
using namespace Gfx;

bool g_paused = false;

#define LOAD_SCENE_DIRECTLY IN_USE

int main( int argc, char* argv[] )
{
    EngineInitInfo engineInitConfig;
    engineInitConfig.windowWidth = engineInitConfig.sceneWidth = 1280;
    engineInitConfig.windowHeight = engineInitConfig.sceneHeight = 720;
	if ( !EngineInitialize( engineInitConfig ) )
    {
        LOG_ERR( "Failed to initialize the engine" );
        return 1;
    }

    TaskGraphBuilder2 builder;
    auto task = builder.AddComputeTask( "task0" );
    TGBBufferRef buf0 = task->AddBufferOutput( "buf0", BUFFER_TYPE_STORAGE, BufferFormat::FLOAT, 1024 );

    task = builder.AddComputeTask( "task1" );
    task->AddBufferInput( buf0 );
    TGBBufferRef buf1 = task->AddBufferOutput( "buf1", BUFFER_TYPE_STORAGE, BufferFormat::FLOAT, 1024 );

    task = builder.AddComputeTask( "task2" );
    task->AddBufferInput( buf1 );
    TGBBufferRef buf2 = task->AddBufferOutput( "buf2", BUFFER_TYPE_STORAGE, BufferFormat::FLOAT, 512 );
    TGBBufferRef buf25 = task->AddBufferOutput( "buf25", BUFFER_TYPE_STORAGE, BufferFormat::FLOAT, 256 );

    task = builder.AddComputeTask( "task3" );
    task->AddBufferInput( buf2 );
    task->AddBufferInput( buf25 );
    TGBBufferRef buf3 = task->AddBufferOutput( "buf3", BUFFER_TYPE_STORAGE, BufferFormat::FLOAT, 1024 );

    task = builder.AddComputeTask( "task4" );
    task->AddBufferInput( buf3 );
    TGBBufferRef buf4 = task->AddBufferOutput( "buf4", BUFFER_TYPE_STORAGE, BufferFormat::FLOAT, 1024 );

    TaskGraphCompileInfo2 compileInfo
    {
        .sceneWidth = 1280,
        .sceneHeight = 720,
        .displayWidth = 1920,
        .displayHeight = 1080,
    };
    TaskGraph taskGraph;
    taskGraph.Compile( builder, compileInfo );
    taskGraph.Free();

    EngineShutdown();

    return 0;

#if USING( LOAD_SCENE_DIRECTLY )
    std::string sceneName = argc > 1 ? argv[1] : "";
    Scene* scene = Scene::Load( PG_ASSET_DIR "scenes/" + sceneName + ".json" );
    if ( !scene )
    {
        EngineShutdown();
        return 1;
    }
    SetPrimaryScene( scene );
#else // #if USING( LOAD_SCENE_DIRECTLY )
    UI::BootMainMenu();
#endif // #else // #if USING( LOAD_SCENE_DIRECTLY )

    Window* window = GetMainWindow();
    window->SetRelativeMouse( USING( LOAD_SCENE_DIRECTLY ) );

    Input::PollEvents();
    Time::Reset();

    while ( !g_engineShutdown )
    {
        window->StartFrame();
        ProcessPendingConsoleCommands();
        AssetManager::ProcessPendingLiveUpdates();
        Input::PollEvents();

        if ( Input::GetKeyDown( Key::ESC ) )
        {
            g_engineShutdown = true;
        }
        if ( Input::GetKeyDown( Key::F1 ) )
        {
            Dvar* debugUIDvar = GetDvar( "r_debugUI" );
            debugUIDvar->Set( !debugUIDvar->GetBool() );
        }
        if ( Input::GetKeyDown( Key::F2 ) )
        {
            Dvar* debugUIDvar = GetDvar( "r_debugUI" );
            debugUIDvar->Set( !debugUIDvar->GetBool() );
        }
        if ( Input::GetKeyDown( Key::UP ) )
        {
            Dvar* r_debugInt = GetDvar( "r_debugInt" );
            r_debugInt->Set( r_debugInt->GetInt() - 1 );
        }
        if ( Input::GetKeyDown( Key::DOWN ) )
        {
            Dvar* r_debugInt = GetDvar( "r_debugInt" );
            r_debugInt->Set( r_debugInt->GetInt() + 1 );
        }

        if ( auto primaryScenePtr = GetPrimaryScene() )
        {
            primaryScenePtr->Update();
        }
        UI::Update();

        RenderSystem::Render();

        //std::this_thread::sleep_for( (std::chrono::milliseconds)30 );

        window->EndFrame();
    }
#if USING( LOAD_SCENE_DIRECTLY )
    delete scene;
#endif // #if USING( LOAD_SCENE_DIRECTLY )

    EngineShutdown();

    return 0;
}
