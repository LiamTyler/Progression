#include "init.hpp"
#include "asset/asset_manager.hpp"
#include "core/lua.hpp"
#include "core/time.hpp"
#if USING( GAME )
#include "core/window.hpp"
#include "core/input.hpp"
#include "renderer/render_system.hpp"
#include "ui/ui_system.hpp"
#endif // #if USING( GAME )
#include "shared/logger.hpp"
#include "shared/random.hpp"
#include <ctime>

namespace PG
{

bool g_engineShutdown  = false;
bool g_headless        = false;
bool g_offlineRenderer = false;

bool EngineInitialize( EngineInitInfo info )
{
    g_headless = info.headless;
    g_offlineRenderer = info.offlineRenderer;
    Logger_Init();
    Logger_AddLogLocation( "stdout", stdout );
    Logger_AddLogLocation( "logfile", "log_engine.txt" );
    Lua::Init();
    AssetManager::Init();
#if USING( GAME )
    if ( !g_headless )
    {
        WindowCreateInfo winCreate;
        winCreate.title   = info.windowTitle;
        winCreate.width   = info.windowWidth;
        winCreate.height  = info.windowHeight;
        winCreate.visible = true;
        winCreate.vsync   = false;
        InitWindowSystem( winCreate );
        Input::Init();
    }
    if ( !RenderSystem::Init( info.sceneWidth, info.sceneHeight, g_headless ) )
    {
        LOG_ERR( "Could not initialize render system" );
        return false;
    }
#endif // #if USING( GAME )
    Time::Reset();
#if USING( GAME )
    if ( !UI::Init() )
    {
        LOG_ERR( "Could not initialize UI system" );
        return false;
    }
#endif // #if USING( GAME )

    return true;
}


void EngineShutdown()
{
#if USING( GAME )
    UI::Shutdown();
#endif // #if USING( GAME )
    AssetManager::Shutdown();
#if USING( GAME )
    RenderSystem::Shutdown();
    if ( !g_headless )
    {
        Input::Shutdown();
        ShutdownWindowSystem();
    }
#endif // #if USING( GAME )
    Lua::Shutdown();
    Logger_Shutdown();
}

} // namespace PG
