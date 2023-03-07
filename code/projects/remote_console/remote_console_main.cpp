#include "shared/logger.hpp"
#include "core/time.hpp"
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
    closesocket( connectSocket ); \
    WSACleanup();                 \
    return 0;

int main( int argc, char* argv[] )
{
    Logger_Init();
    Logger_AddLogLocation( "stdout", stdout );

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
        std::string line;
        std::getline( std::cin, line );
        iResult = send( connectSocket, line.c_str(), (int)line.length(), 0 );
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