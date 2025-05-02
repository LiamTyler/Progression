#include "console_commands.hpp"
#include "shared/platform_defines.hpp"

using namespace PG;

static ConsoleCmd s_commands[] = {
    {"loadFF", "one argument expected: the name of the fastfile to load/reload"},
    {"exit",   "exit the application"                                          },
    {"quit",   "exit the application"                                          },
};

std::vector<ConsoleCmd> PG::GetConsoleCommands()
{
    std::vector<ConsoleCmd> ret;
    ret.resize( ARRAY_COUNT( s_commands ) );
    for ( i32 i = 0; i < ARRAY_COUNT( s_commands ); ++i )
    {
        ret[i] = { s_commands[i].name, s_commands[i].usage };
    }

    return ret;
}

#if !USING( REMOTE_CONSOLE_CLIENT )
#include "asset/asset_manager.hpp"
#include "core/dvars.hpp"
#include "core/engine_globals.hpp"
#include "shared/logger.hpp"
#include "shared/platform_defines.hpp"
#include "shared/string.hpp"
#include <functional>
#include <mutex>

static std::mutex s_lock;
std::vector<std::string> s_pendingCommands;

static void Process_loadFF( std::string_view* args, u32 numArgs )
{
    if ( numArgs != 1 )
    {
        LOG_ERR( "The 'loadFF' command expects 1 arg (the name of the fastfile to load)" );
        return;
    }

    AssetManager::LoadFastFile( std::string( args[0] ) );
}

static void Process_quit( std::string_view* args, u32 numArgs ) { eg.shutdown = true; }

using CommandFunc = std::function<void( std::string_view*, u32 )>;

static CommandFunc s_commandFunctions[] = { Process_loadFF, Process_quit, Process_quit };
static_assert( ARRAY_COUNT( s_commands ) == ARRAY_COUNT( s_commandFunctions ) );

static void ProcessCommand( const std::string& cmdStr )
{
    std::vector<std::string_view> subStrs = SplitString( cmdStr, " " );
    u32 numArgs                           = subStrs.empty() ? 0 : static_cast<u32>( subStrs.size() - 1 );
    std::string_view cmd                  = subStrs.empty() ? "" : subStrs[0];
    for ( i32 i = 0; i < ARRAY_COUNT( s_commands ); ++i )
    {
        if ( cmd == s_commands[i].name )
        {
            s_commandFunctions[i]( subStrs.data() + 1, numArgs );
            return;
        }
    }
    const auto& dvarMap = GetAllDvars();
    for ( const auto& [dvarName, dvarPtr] : dvarMap )
    {
        if ( cmd == dvarName )
        {
            if ( numArgs == 0 )
            {
                LOG_ERR( "Dvar %s specified on console command, but no value was given after dvar name", dvarName.data() );
                return;
            }

            dvarPtr->SetFromString( std::string( subStrs[1] ) );
            return;
        }
    }

    LOG_ERR( "Unknown console command '%s'", cmdStr.c_str() );
}

namespace PG
{

void ProcessPendingConsoleCommands()
{
    s_lock.lock();
    const auto pendingCmds = s_pendingCommands;
    s_pendingCommands.clear();
    s_lock.unlock();

    for ( size_t i = 0; i < pendingCmds.size(); ++i )
    {
        ProcessCommand( pendingCmds[i] );
    }
}

void AddConsoleCommand( const std::string& cmd )
{
    std::scoped_lock lock( s_lock );
    s_pendingCommands.push_back( cmd );
}

} // namespace PG

#endif // #if !USING( REMOTE_CONSOLE_CLIENT )
