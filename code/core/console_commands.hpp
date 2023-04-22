#pragma once

#include <string>

namespace PG
{

void ProcessPendingConsoleCommands();
void AddConsoleCommand( const std::string& cmd );

} // namespace PG
