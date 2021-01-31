#include "engine_init.hpp"
#include "asset/asset_manager.hpp"
#include "core/input.hpp"
#include "core/lua.hpp"
#include "core/time.hpp"
#include "core/window.hpp"
#include "renderer/render_system.hpp"
#include "utils/logger.hpp"
#include "utils/random.hpp"
#include <ctime>

namespace PG
{

bool g_engineShutdown = false;
bool g_headless       = false;

bool EngineInitialize( const EngineInitInfo& info )
{
    g_headless = info.headless;
    Logger_Init();
    Logger_AddLogLocation( "stdout", stdout );
    Random::SetSeed( time( NULL ) );
    Lua::Init( Lua::g_LuaState );

    if ( !g_headless )
    {
        WindowCreateInfo winCreate;
        winCreate.title   = info.windowTitle;
        winCreate.width   = 1280;
        winCreate.height  = 720;
        winCreate.visible = true;
        winCreate.vsync   = false;
        InitWindowSystem( winCreate );
        Input::Init();
    }
    
    AssetManager::Init();
    if ( !RenderSystem::Init( g_headless ) )
    {
        LOG_ERR( "Could not initialize render system" );
        return false;
    }
    Time::Reset();

    return true;
}


void EngineShutdown()
{
    AssetManager::Shutdown();
    RenderSystem::Shutdown();
    if ( !g_headless )
    {
        Input::Shutdown();
        ShutdownWindowSystem();
    }
    Logger_Shutdown();
}

} // namespace PG
