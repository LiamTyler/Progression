#include "remote_console_server.hpp"

#if USING( REMOTE_CONSOLE )

#include "shared/logger.hpp"
#include <thread>
#if USING ( WINDOWS_PROGRAM )
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment (lib, "Ws2_32.lib")
#else // #if USING ( WINDOWS_PROGRAM )
#error "Need to implement socket code for linux"
#endif // #else // #if USING ( WINDOWS_PROGRAM )

static bool s_serverShouldStop;
static SOCKET s_clientSocket;
static SOCKET s_listenSocket;
static std::thread s_serverThread;
static bool s_initialized;

static void ListenForCommands();

namespace PG::RemoteConsoleServer
{


bool Init()
{
    s_initialized = false;
    s_serverShouldStop = false;

    WSADATA wsa;
    int iResult;

    s_listenSocket = INVALID_SOCKET;
    s_clientSocket = INVALID_SOCKET;

    struct addrinfo *result = NULL;
    struct addrinfo hints;
    
	if ( WSAStartup( MAKEWORD( 2,2 ), &wsa ) != 0 )
	{
        LOG_ERR( "Failed. Error Code : %d", WSAGetLastError() );
		return false;
	}
	
    ZeroMemory( &hints, sizeof( hints ) );
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags    = AI_PASSIVE;

    iResult = getaddrinfo( NULL, "27015", &hints, &result );
    if ( iResult != 0 )
    {
        LOG_ERR( "getaddrinfo failed with error: %d", iResult );
        WSACleanup();
        return false;
    }

    s_listenSocket = socket( result->ai_family, result->ai_socktype, result->ai_protocol );
    if ( s_listenSocket == INVALID_SOCKET )
    {
        LOG_ERR( "socket failed with error: %ld", WSAGetLastError() );
        freeaddrinfo( result );
        WSACleanup();
        return false;
    }

    iResult = bind( s_listenSocket, result->ai_addr, (int)result->ai_addrlen );
    if ( iResult == SOCKET_ERROR )
    {
        LOG_ERR( "bind failed with error: %d", WSAGetLastError() );
        freeaddrinfo( result );
        closesocket( s_listenSocket );
        WSACleanup();
        return false;
    }

    freeaddrinfo( result );

    iResult = listen( s_listenSocket, 1 );
    if ( iResult == SOCKET_ERROR )
    {
        LOG_ERR( "Listen failed with error: %d", WSAGetLastError() );
        closesocket( s_listenSocket );
        WSACleanup();
        return false;
    }
    s_initialized = true;
    s_serverThread = std::thread( ListenForCommands );

    return true;
}


void Shutdown()
{
    if ( s_initialized )
    {
        LOG( "Shutting down remote console" );
        s_serverShouldStop = true;
        if ( s_clientSocket != INVALID_SOCKET )
        {
            int result = shutdown( s_clientSocket, SD_SEND );
            if ( result == SOCKET_ERROR )
            {
                LOG_ERR( "Failed to shutdown client socket with error: %d", WSAGetLastError() );
            }
            closesocket( s_clientSocket );
        }
        closesocket( s_listenSocket );
        s_serverThread.join();
        WSACleanup();
    }
    s_initialized = false;
}


} // namespace PG::RemoteConsoleServer


static void ListenForCommands() 
{
    while ( !s_serverShouldStop )
    {
        LOG( "RemoteConsoleServer: Waiting for client..." );
        s_clientSocket = accept( s_listenSocket, NULL, NULL );
        if ( s_clientSocket == INVALID_SOCKET )
        {
            if ( !s_serverShouldStop )
            {
                LOG_ERR( "RemoteConsoleServer: Accept failed with error: %d", WSAGetLastError() );
            }
            continue;
        }
        LOG( "RemoteConsoleServer: client connected" );

        // make recv unblock itself every so often to see if the program is trying to shutdown
        timeval timeout;
        timeout.tv_sec  = 200; // milliseconds
        timeout.tv_usec = 0;
        if ( setsockopt( s_clientSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof( timeout ) ) == SOCKET_ERROR )
        {
            LOG_ERR( "RemoteConsoleServer: could not set client socket to be non-blocking!" );
        }

        const int recvBufferLen = 1024;
        char recvBuffer[recvBufferLen];
        bool clientConnected = true;
        while ( clientConnected && !s_serverShouldStop )
        {
            int bytesReceived = recv( s_clientSocket, recvBuffer, recvBufferLen, 0 );

            if ( bytesReceived > 0 )
            {
                recvBuffer[bytesReceived] = '\0';
                LOG( "RemoteConsoleServer: recieved message '%s'", recvBuffer );

                //std::string sendMsg = "message recieved";
                //int iSendResult = send( s_clientSocket, sendMsg.c_str(), (int)sendMsg.length() + 1, 0 );
                //if ( iSendResult == SOCKET_ERROR )
                //{
                //    LOG_ERR( "send failed with error: %d", WSAGetLastError() );
                //    clientConnected = false;
                //}
            }
            else 
            {
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
                clientConnected = false;
            }
        }

        if ( s_serverShouldStop )
            LOG( "RemoteConsoleServer: closing client socket" );
        closesocket( s_clientSocket );
    }
}

#endif // #if USING( REMOTE_CONSOLE )