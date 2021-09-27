#pragma once

#include <string>

namespace PG
{

extern bool g_engineShutdown;

struct EngineInitInfo
{
    bool headless = false;
    std::string windowTitle = "Untitled";
    uint32_t windowWidth = 1920;
    uint32_t windowHeight = 1080;
    uint32_t sceneWidth = 1280;
    uint32_t sceneHeight = 720;
};

bool EngineInitialize( const EngineInitInfo& info );

void EngineShutdown();

} // namespace PG