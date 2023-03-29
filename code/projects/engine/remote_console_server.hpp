#pragma once

#include "shared/platform_defines.hpp"

#if USING( GAME ) && !USING( SHIP_BUILD ) && USING( WINDOWS_PROGRAM )
#define REMOTE_CONSOLE IN_USE
#else // #if USING( GAME ) && !USING( SHIP_BUILD ) && USING( WINDOWS_PROGRAM )
#define REMOTE_CONSOLE NOT_IN_USE
#endif // #else // #if USING( GAME ) && !USING( SHIP_BUILD ) && USING( WINDOWS_PROGRAM )

namespace PG::RemoteConsoleServer
{

#if USING( REMOTE_CONSOLE )
bool Init();
void Shutdown();
#else // #if USING( REMOTE_CONSOLE )
inline bool Init() { return true; }
inline void Shutdown() {}
#endif // #else // #if USING( REMOTE_CONSOLE )

} // namespace PG::RemoteConsoleServer
