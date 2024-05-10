#pragma once

namespace PG
{

struct EngineGlobals
{
    bool shutdown;
    bool resizeRequested;
    bool headless;
};

extern EngineGlobals eg;

} // namespace PG
