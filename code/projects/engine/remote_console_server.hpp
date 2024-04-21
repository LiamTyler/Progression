#pragma once

#include "shared/platform_defines.hpp"

#define REMOTE_CONSOLE USE_IF( USING( GAME ) && !USING( SHIP_BUILD ) && USING( WINDOWS_PROGRAM ) )

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
