#pragma once

#include <string>
#include <vector>

namespace PG
{

void ProcessPendingConsoleCommands();
void AddConsoleCommand( const std::string& cmd );

struct ConsoleCmd
{
    std::string_view name;
    std::string_view usage;
};
std::vector<ConsoleCmd> GetConsoleCommands();

} // namespace PG
