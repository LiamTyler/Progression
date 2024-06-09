#include "init.hpp"
#include "asset/asset_manager.hpp"
#include "core/dvars.hpp"
#include "core/engine_globals.hpp"
#include "core/lua.hpp"
#include "core/time.hpp"
#if USING( GAME )
#include "core/input.hpp"
#include "core/scene.hpp"
#include "core/window.hpp"
#include "projects/engine/remote_console_server.hpp"
#include "renderer/render_system.hpp"
#include "ui/ui_system.hpp"
#endif // #if USING( GAME )
#include "shared/logger.hpp"
#include "shared/random.hpp"
#include <ctime>

#if !USING( GAME )
#include <omp.h>
#endif // #if !USING( GAME )

namespace PG
{

bool EngineInitialize( EngineInitInfo info )
{
#if !USING( GAME )
    omp_set_nested( 1 );
#endif // #if !USING( GAME )

    eg.headless = info.headless;
    Logger_Init();
    Logger_AddLogLocation( "stdout", stdout );
#if USING( GAME )
    Logger_AddLogLocation( "logfile", PG_BIN_DIR "log_engine.txt" );
#endif // #if USING( GAME )
    Lua::Init();
    Time::Reset();
    AssetManager::Init();
#if USING( GAME )
    u32 framebufferWidth  = info.windowWidth;
    u32 framebufferHeight = info.windowHeight;
    if ( !eg.headless )
    {
        WindowCreateInfo winCreate;
        winCreate.title   = info.windowTitle;
        winCreate.width   = info.windowWidth;
        winCreate.height  = info.windowHeight;
        winCreate.visible = true;
        winCreate.vsync   = false;
        InitWindowSystem( winCreate );
        Input::Init();
        framebufferWidth  = GetMainWindow()->FramebufferWidth();
        framebufferHeight = GetMainWindow()->FramebufferHeight();
    }
    if ( !RenderSystem::Init( info.sceneWidth, info.sceneHeight, framebufferWidth, framebufferHeight, eg.headless ) )
    {
        LOG_ERR( "Could not initialize render system" );
        return false;
    }
// if ( !UI::Init() )
//{
//     LOG_ERR( "Could not initialize UI system" );
//     return false;
// }
#if !USING( SHIP_BUILD )
    ExportDvars();
#endif // #if !USING( SHIP_BUILD )
    RemoteConsoleServer::Init();
#endif // #if USING( GAME )

#if USING( GAME )
    if ( !eg.headless )
        Input::ResetMousePosition();
#endif // #if USING( GAME )

    return true;
}

void EngineShutdown()
{
#if USING( GAME )
    FreeAllScenes();
    RemoteConsoleServer::Shutdown();
    // UI::Shutdown();
    RenderSystem::Shutdown();
    if ( !eg.headless )
    {
        Input::Shutdown();
        ShutdownWindowSystem();
    }
#endif // #if USING( GAME )
    AssetManager::Shutdown();
    Lua::Shutdown();
    Logger_Shutdown();
}

} // namespace PG
