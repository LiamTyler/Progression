#include "engine_init.hpp"
#include "asset/asset_manager.hpp"
#include "core/input.hpp"
#include "core/time.hpp"
#include "core/window.hpp"
#include "renderer/render_system.hpp"
#include "utils/logger.hpp"
#include "utils/random.hpp"
#include <ctime>

namespace PG
{

bool g_engineShutdown = false;
bool g_converterMode  = false;

bool EngineInitialize()
{
    
    Logger_Init();
    Logger_AddLogLocation( "stdout", stdout );
    Random::SetSeed( time( NULL ) );

    // window
    {
        WindowCreateInfo winCreate;
        winCreate.title   = "Scene Viewer";
        winCreate.width   = 1280;
        winCreate.height  = 720;
        winCreate.visible = true;
        winCreate.vsync   = false;
        InitWindowSystem( winCreate );
    }
    Time::Reset();
    Input::Init();
    AssetManager::Init();
    if ( !RenderSystem::Init() )
    {
        LOG_ERR( "Could not initialize render system" );
        return false;
    }

    return true;
}


void EngineShutdown()
{
    AssetManager::Shutdown();
    RenderSystem::Shutdown();
    Input::Shutdown();
    ShutdownWindowSystem();
    Logger_Shutdown();
}

} // namespace PG
