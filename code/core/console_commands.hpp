#pragma once

#include <string>

namespace PG
{

void ProcessPendingCommands();
void AddCommand( const std::string& cmd );

} // namespace PG
