#pragma once

#include <string>

namespace PG
{

extern bool g_engineShutdown;
extern bool g_headless;
extern bool g_offlineRenderer;

struct EngineInitInfo
{
    bool headless           = false;
    bool offlineRenderer    = false;
    std::string windowTitle = "Untitled";
    uint32_t windowWidth    = 1920;
    uint32_t windowHeight   = 1080;
    uint32_t sceneWidth     = 1280;
    uint32_t sceneHeight    = 720;
};

bool EngineInitialize( EngineInitInfo info = {} );

void EngineShutdown();

} // namespace PG
