#pragma once

#include "shared/platform_defines.hpp"

#define REMOTE_CONSOLE_SERVER USE_IF( USING( GAME ) && USING( DEVELOPMENT_BUILD ) && USING( WINDOWS_PROGRAM ) )

namespace PG::RemoteConsoleServer
{

#if USING( REMOTE_CONSOLE_SERVER )
bool Init();
void Shutdown();
#else  // #if USING( REMOTE_CONSOLE_SERVER )
inline bool Init() { return true; }
inline void Shutdown() {}
#endif // #else // #if USING( REMOTE_CONSOLE_SERVER )

} // namespace PG::RemoteConsoleServer
