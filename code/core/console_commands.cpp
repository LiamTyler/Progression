#include "core/console_commands.hpp"
#include "asset/asset_manager.hpp"
#include "shared/logger.hpp"
#include "shared/platform_defines.hpp"
#include "shared/string.hpp"
#include <functional>
#include <mutex>

using namespace PG;


static std::mutex s_lock;
std::vector<std::string> s_pendingCommands;

static void Process_loadFF( std::string* args, uint32_t numArgs )
{
    if ( numArgs != 1 )
    {
        LOG_ERR( "The 'loadFF' command expects 1 arg (the name of the fastfile to load" );
        return;
    }

    AssetManager::LoadFastFile( args[0] );
}


struct RegisteredCommand
{
    const char* const name;
    std::function<void( std::string*, uint32_t )> func;
};

static RegisteredCommand s_commands[] =
{
    { "loadFF", Process_loadFF }
};


void ProcessCommand( const std::string& cmdStr )
{
    std::vector<std::string> subStrs = SplitString( cmdStr, " " );
    uint32_t numArgs = subStrs.empty() ? 0 : static_cast<uint32_t>( subStrs.size() - 1 );
    const std::string& cmd = subStrs.empty() ? "" : subStrs[0];
    for ( int i = 0; i < ARRAY_COUNT( s_commands ); ++i )
    {
        if ( !strcmp( cmd.c_str(), s_commands[i].name ) )
        {
            s_commands[i].func( subStrs.data() + 1, numArgs );
            return;
        }
    }
    LOG_ERR( "Unknown console command '%s'", cmdStr.c_str() );
}

void PG::ProcessPendingCommands()
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

void PG::AddCommand( const std::string& cmd )
{
    std::scoped_lock( s_lock );
    s_pendingCommands.push_back( cmd );
}