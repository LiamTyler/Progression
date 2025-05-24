#include "core/console_commands.hpp"
#include "core/time.hpp"
#include "isocline/include/isocline.h"
#include "shared/logger.hpp"
#include "shared/sockets.hpp"
#include "shared/string.hpp"
#include <fstream>
#include <iostream>

using namespace PG;

struct Command
{
    std::string name;
    std::string description;
};

static std::vector<const char*> s_commandNames;
static std::vector<Command> s_commands;
static bool LoadCommands();
static void Completer( ic_completion_env_t* cenv, const char* prefix );

void SendToServer( int argc, char* argv[] )
{
    ClientSocket clientSocket;
    if ( !clientSocket.OpenSocket( "localhost", 27015 ) )
        return;
    if ( !clientSocket.OpenConnection() )
        return;

    LOG( "Connected to game" );

    if ( argc > 1 )
    {
        std::string cmd = argv[1];
        for ( int i = 2; i < argc; ++i )
            cmd += " " + std::string( argv[i] );

        clientSocket.SendData( cmd.c_str(), (int)cmd.length() );
        return;
    }

    while ( true )
    {
        char* input = ic_readline( ">" );
        if ( input == NULL )
            return;

        std::string sInput( input );
        if ( sInput == "exit" )
        {
            free( input );
            return;
        }

        if ( sInput.empty() )
            continue;

        std::vector<std::string_view> words = SplitString( sInput, " " );
        if ( words.size() && words[0] == "help" )
        {
            if ( words.size() < 2 )
            {
                ic_println( "Usage: help [name_of_command]" );
                continue;
            }

            bool found = false;
            for ( const Command& cmd : s_commands )
            {
                if ( cmd.name == words[1] )
                {
                    ic_printf( "Description for command %s:\n%s\n\n", cmd.name.c_str(), cmd.description.c_str() );
                    found = true;
                    break;
                }
            }

            if ( !found )
            {
                ic_printf( "Help: no command with name '%s' found\n", words[1].data() );
            }
            continue;
        }

        if ( !clientSocket.SendData( input, (int)strlen( input ) ) )
            return;
    }
}

int main( int argc, char* argv[] )
{
    Logger_Init();
    Logger_AddLogLocation( "stdout", stdout );

    bool singleCommand = argc > 1;
    if ( !singleCommand && !LoadCommands() )
    {
        LOG_ERR( "Invalid dvar file. Please regenerate (rerun engine)" );
        return 0;
    }

    if ( !singleCommand )
    {
        setlocale( LC_ALL, "C.UTF-8" );

        // use `ic_print` functions to use bbcode's for markup
        ic_style_def( "kbd", "gray underline" );    // you can define your own styles
        ic_style_def( "ic-prompt", "ansi-maroon" ); // or re-define system styles

        ic_printf( "[b]Remote Console[/b]:\n"
                   "- Type 'exit' to quit. (or use [kbd]ctrl-d[/]).\n"
                   "- Press [kbd]F1[/] for help on editing commands.\n"
                   "- Use [kbd]ctrl-r[/] to search the history.\n\n" );

        // enable history; use a NULL filename to not persist history to disk
        ic_set_history( PG_BIN_DIR "remote_console_history.txt", -1 /* default entries (= 200) */ );

        // enable completion with a default completion function
        ic_set_default_completer( &Completer, NULL );

        // enable syntax highlighting with a highlight function
        // ic_set_default_highlighter(highlighter, NULL);

        // try to auto complete after a completion as long as the completion is unique
        ic_enable_auto_tab( true );

        // inline hinting is enabled by default
        // ic_enable_hint(false);
    }

    if ( !InitSocketsLib() )
        return 0;

    SendToServer( argc, argv );

    ShutdownSocketLib();

    return 0;
}

static bool LoadCommands()
{
    const std::vector<ConsoleCmd> consoleCmds = GetConsoleCommands();
    s_commands.reserve( consoleCmds.size() );
    for ( const ConsoleCmd& cmd : consoleCmds )
    {
        Command c;
        c.name        = cmd.name;
        c.description = cmd.usage;
        s_commands.emplace_back( c );
    }

    // dvars
    // Make sure this filename lines up with the filename in dvars.cpp
    std::ifstream dvarFile( PG_BIN_DIR "../dvars_exported.bin", std::ios::binary );
    if ( dvarFile )
    {
        int numDvars;
        dvarFile.read( (char*)&numDvars, sizeof( numDvars ) );
        for ( int dvarIdx = 0; dvarIdx < numDvars; ++dvarIdx )
        {
            Command& cmd = s_commands.emplace_back();
            int nameLen;
            dvarFile.read( (char*)&nameLen, sizeof( nameLen ) );
            cmd.name.resize( nameLen );
            dvarFile.read( cmd.name.data(), nameLen );

            int descLen;
            dvarFile.read( (char*)&descLen, sizeof( descLen ) );
            cmd.description.resize( descLen );
            dvarFile.read( cmd.description.data(), descLen );
        }
    }
    else
    {
        LOG_WARN( "No dvar export file found (generated when the engine runs). No autocomplete for dvars will be available" );
    }

    s_commandNames.resize( s_commands.size() );
    for ( size_t i = 0; i < s_commands.size(); ++i )
    {
        s_commandNames[i] = s_commands[i].name.c_str();
    }

    s_commandNames.push_back( NULL );

    return true;
}

static void Word_completer( ic_completion_env_t* cenv, const char* word ) { ic_add_completions( cenv, word, s_commandNames.data() ); }

static void Completer( ic_completion_env_t* cenv, const char* input )
{
    // try to complete file names from the roots "." and "/usr/local"
    // ic_complete_filename( cenv, input, 0, ".;/usr/local;c:\\Program Files" , NULL /* any extension */ );

    // and also use our custom completer
    ic_complete_word( cenv, input, &Word_completer, NULL /* from default word boundary; whitespace or separator */ );

    // ic_complete_word( cenv, input, &word_completer, &ic_char_is_idletter );
    // ic_complete_qword( cenv, input, &word_completer, &ic_char_is_idletter  );
}
