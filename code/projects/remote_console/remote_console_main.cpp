#include "shared/logger.hpp"
#include "core/time.hpp"
#include "isocline/include/isocline.h"
#include <fstream>
#include <iostream>

#if USING ( WINDOWS_PROGRAM )
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment (lib, "Ws2_32.lib")
#else // #if USING ( WINDOWS_PROGRAM )
#error "Need to implement socket code for linux"
#endif // #else // #if USING ( WINDOWS_PROGRAM )

using namespace PG;

#define CLOSE_SOCKET_AND_RETURN   \
{                                 \
    closesocket( connectSocket ); \
    WSACleanup();                 \
    Logger_Shutdown();            \
    return 0;                     \
}

struct Command
{
    std::string name;
    std::string description;
};

static std::vector<const char*> s_commandNames;
static std::vector<Command> s_commands;
static bool LoadCommands();
static void Completer( ic_completion_env_t* cenv, const char* prefix );

int main( int argc, char* argv[] )
{
    Logger_Init();
    Logger_AddLogLocation( "stdout", stdout );
    
    if ( !LoadCommands() )
    {
        LOG_ERR( "Invalid dvar file. Please regenerate (rerun engine)" );
        return 0;
    }

    setlocale( LC_ALL, "C.UTF-8" );

    // use `ic_print` functions to use bbcode's for markup
    ic_style_def( "kbd","gray underline" );     // you can define your own styles
    ic_style_def( "ic-prompt","ansi-maroon" );  // or re-define system styles
  
    ic_printf( "[b]Remote Console[/b]:\n"
            "- Type 'exit' to quit. (or use [kbd]ctrl-d[/]).\n"
            "- Press [kbd]F1[/] for help on editing commands.\n"
            "- Use [kbd]ctrl-r[/] to search the history.\n\n" );
  
    // enable history; use a NULL filename to not persist history to disk
    ic_set_history( PG_BIN_DIR "remote_console_history.txt", -1 /* default entries (= 200) */);

    // enable completion with a default completion function
    ic_set_default_completer( &Completer, NULL );

    // enable syntax highlighting with a highlight function
    //ic_set_default_highlighter(highlighter, NULL);

    // try to auto complete after a completion as long as the completion is unique
    ic_enable_auto_tab( true );

    // inline hinting is enabled by default
    // ic_enable_hint(false);

    {
        WSADATA wsaData;
        int iResult = WSAStartup( MAKEWORD( 2,2 ), &wsaData );
        if ( iResult != 0 )
        {
            LOG_ERR( "WSAStartup failed with error: %d\n", iResult );
            return 0;
        }

        LOG( "Connecting to game..." );
        struct addrinfo hints;
        ZeroMemory( &hints, sizeof( hints ) );
        hints.ai_family   = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        struct addrinfo *addr = NULL;
        iResult = getaddrinfo( NULL, "27015", &hints, &addr );
        if ( iResult != 0 )
        {
            LOG_ERR( "getaddrinfo failed with error: %d", iResult );
            WSACleanup();
            return 0;
        }

        SOCKET connectSocket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
        if ( connectSocket == INVALID_SOCKET )
        {
            LOG_ERR( "Failed to create socket with error: %ld", WSAGetLastError() );
            WSACleanup();
            return 0;
        }

        iResult = connect( connectSocket, addr->ai_addr, (int)addr->ai_addrlen );
        freeaddrinfo( addr );
        if ( iResult == SOCKET_ERROR || connectSocket == INVALID_SOCKET )
        {
            LOG_ERR( "Unable to connect to game" );
            CLOSE_SOCKET_AND_RETURN;
        }

        LOG( "Connected to game" );

        // pre-created single commands
        if ( argc > 1 )
        {
            std::string cmd = argv[1];
            for ( int i = 2; i < argc; ++i )
                cmd += " " + std::string( argv[i] );

            iResult = send( connectSocket, cmd.c_str(), (int)cmd.length(), 0 );
            if ( iResult == SOCKET_ERROR )
            {
                int err = WSAGetLastError();
                if ( err == WSAECONNRESET )
                    LOG_ERR( "Game was shutdown! Closing remote console" );
                else
                    LOG_ERR( "Failed to send command to game with error: %d. Closing remote console", WSAGetLastError() );
            }
            CLOSE_SOCKET_AND_RETURN;
        }

        while ( true )
        {
            char* input = ic_readline( ">" );
            if ( input == NULL )
                CLOSE_SOCKET_AND_RETURN;

            bool shutdown = !strcmp( input, "exit" );
            if ( shutdown )
            {
                free( input );
                CLOSE_SOCKET_AND_RETURN;
            }

            int msgLen = (int)strlen( input );
            if ( !msgLen )
                continue;

            iResult = send( connectSocket, input, (int)strlen( input ), 0 );
            free( input );
            if ( iResult == SOCKET_ERROR )
            {
                int err = WSAGetLastError();
                if ( err == WSAECONNRESET )
                    LOG_ERR( "Game was shutdown! Closing remote console" );
                else
                    LOG_ERR( "Failed to send command to game with error: %d. Closing remote console", WSAGetLastError() );
                CLOSE_SOCKET_AND_RETURN;
            }
        }

        // shutdown the connection since no more data will be sent
        iResult = shutdown( connectSocket, SD_SEND );
        if ( iResult == SOCKET_ERROR )
        {
            LOG_ERR( "shutdown failed with error: %d", WSAGetLastError() );
            CLOSE_SOCKET_AND_RETURN;
        }

        CLOSE_SOCKET_AND_RETURN;
    }
}

static bool LoadCommands()
{
    // baked in commands, from console_commands.cpp
    s_commands.push_back( { "loadFF", "1 arg expected: the name of the fastfile to load/reload" } );

    // dvars
    // Make sure this filename lines up with the filename in dvars.cpp
    std::ifstream dvarFile( PG_BIN_DIR "../dvars_exported.bin", std::ios::binary );
    if ( dvarFile )
    {
        int numDvars;
        dvarFile.read( (char*)&numDvars, sizeof( numDvars ) );
        for ( int dvarIdx = 0; dvarIdx < numDvars; ++dvarIdx )
        {
            Command &cmd = s_commands.emplace_back();
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

static void Word_completer( ic_completion_env_t* cenv, const char* word ) 
{
    ic_add_completions( cenv, word, s_commandNames.data() );
}

static void Completer( ic_completion_env_t* cenv, const char* input ) 
{
    // try to complete file names from the roots "." and "/usr/local"
    //ic_complete_filename( cenv, input, 0, ".;/usr/local;c:\\Program Files" , NULL /* any extension */ );

    // and also use our custom completer  
    ic_complete_word( cenv, input, &Word_completer, NULL /* from default word boundary; whitespace or separator */ );        
  
    // ic_complete_word( cenv, input, &word_completer, &ic_char_is_idletter );        
    // ic_complete_qword( cenv, input, &word_completer, &ic_char_is_idletter  );        
}