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
    uint32_t windowWidth    = 1920;
    uint32_t windowHeight   = 1080;
    uint32_t sceneWidth     = 1280;
    uint32_t sceneHeight    = 720;
};

bool EngineInitialize( EngineInitInfo info = {} );

void EngineShutdown();

} // namespace PG
