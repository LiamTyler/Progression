#include "remote_console_server.hpp"
#include "core/console_commands.hpp"

#if USING( REMOTE_CONSOLE_SERVER )

#include "shared/logger.hpp"
#include "shared/sockets.hpp"
#include <thread>

static bool s_serverShouldStop;
static ServerSocket s_serverSocket;
static ClientSocket s_clientSocket;
static std::thread s_serverThread;
static bool s_initialized;

static void ListenForCommands();

namespace PG::RemoteConsoleServer
{

bool Init()
{
    s_initialized      = false;
    s_serverShouldStop = false;

    InitSocketsLib();
    if ( !s_serverSocket.Open( "localhost", 27015 ) )
        return false;

    s_initialized  = true;
    s_serverThread = std::thread( ListenForCommands );

    return true;
}

void Shutdown()
{
    if ( s_initialized )
    {
        LOG( "Shutting down remote console" );
        s_serverShouldStop = true;
        s_serverThread.join();
        s_serverSocket.Close();
    }
    s_initialized = false;
}

} // namespace PG::RemoteConsoleServer

static void ListenForCommands()
{
    while ( !s_serverShouldStop )
    {
        LOG( "RemoteConsoleServer: Waiting for client..." );
        if ( !s_serverSocket.AcceptConnection( s_clientSocket ) )
        {
            // if ( !s_serverShouldStop )
            //{
            //     LOG_ERR( "RemoteConsoleServer: Accept failed with error: %d" );
            // }
            continue;
        }
        LOG( "RemoteConsoleServer: client connected" );

        // make recv unblock itself every so often to see if the program is trying to shutdown
        s_clientSocket.SetNonblockingRecv( 50 );

        const int recvBufferLen = 1024;
        char recvBuffer[recvBufferLen];
        bool clientConnected = true;
        while ( clientConnected && !s_serverShouldStop )
        {
            int bytesReceived = s_clientSocket.ReceiveData( recvBuffer, recvBufferLen );
            if ( bytesReceived > 0 )
            {
                recvBuffer[bytesReceived] = '\0';
                PG::AddConsoleCommand( recvBuffer );
            }
            else
            {
#if USING( WINDOWS_PROGRAM )
                int err = WSAGetLastError();
                if ( err == WSAETIMEDOUT || errno == WSAEWOULDBLOCK )
                    continue;

                if ( bytesReceived == 0 || err == WSAECONNRESET )
                {
                    LOG( "RemoteConsoleServer: client closed connection" );
                }
                else if ( err != WSAEINTR )
                {
                    LOG_ERR( "RemoteConsoleServer: recv failed with error: %d. Closing connection", WSAGetLastError() );
                }
#else  // #if USING( WINDOWS_PROGRAM )
                LOG_ERR( "RemoteConsoleServer: client closed connection" );
#endif // #else // #if USING( WINDOWS_PROGRAM )
                clientConnected = false;
            }
        }

        if ( s_serverShouldStop )
            LOG( "RemoteConsoleServer: closing client socket" );
        s_clientSocket.Close();
    }
}

#endif // #if USING( REMOTE_CONSOLE_SERVER )
