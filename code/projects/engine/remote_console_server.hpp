#pragma once

#include "shared/platform_defines.hpp"

#if USING( GAME ) && !USING( SHIP_BUILD )
#define REMOTE_CONSOLE IN_USE
#else // #if USING( GAME ) && !USING( SHIP_BUILD )
#define REMOTE_CONSOLE IN_USE
#endif // #else // #if USING( GAME ) && !USING( SHIP_BUILD )

namespace PG::RemoteConsoleServer
{

#if USING( REMOTE_CONSOLE )
bool Init();
void Shutdown();
#else // #if USING( REMOTE_CONSOLE )
bool Init() { return true; }
void Shutdown() {}
#endif // #else // #if USING( REMOTE_CONSOLE )

} // namespace PG::RemoteConsoleServer