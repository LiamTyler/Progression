#pragma once

#include <string>

namespace PG
{

extern bool g_engineShutdown;

struct EngineInitInfo
{
    bool headless = false;
    std::string windowTitle = "Untitled";
};

bool EngineInitialize( const EngineInitInfo& info );

void EngineShutdown();

} // namespace PG