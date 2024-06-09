#pragma once

#include "asset/asset_manager.hpp"
#include "core/dvars.hpp"
#include "core/time.hpp"
#include "shared/platform_defines.hpp"

#if USING( GAME )
#include "core/input.hpp"
#include "core/window.hpp"
#endif // #if USING( GAME )

namespace PG
{

struct EngineInitInfo
{
    bool headless           = false;
    std::string windowTitle = "Untitled";
    u32 windowWidth         = 1920;
    u32 windowHeight        = 1080;
    u32 sceneWidth          = 1280;
    u32 sceneHeight         = 720;
};

bool EngineInitialize( EngineInitInfo info = {} );

void EngineShutdown();

} // namespace PG
