#pragma once

#include "platform_defines.hpp"

#if USING( WINDOWS_PROGRAM )
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment( lib, "Ws2_32.lib" )
#define ADDRINFO struct addrinfo
#else // #if USING( WINDOWS_PROGRAM )
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
using SOCKET = int;
static constexpr int INVALID_SOCKET = -1;
static constexpr int SOCKET_ERROR = -1;
#endif // #else // #if USING( WINDOWS_PROGRAM )

class ClientSocket
{
    friend class ServerSocket;

private:
    struct addrinfo* m_addr = nullptr;
    SOCKET m_connectSocket  = INVALID_SOCKET;

public:
    ClientSocket() = default;
    ~ClientSocket();

    bool OpenSocket( const char* host, const int port );
    bool OpenConnection();
    bool Close();

    bool SendData( const void* data, int sizeInBytes );
    int ReceiveData( void* buffer, int bufferSizeInBytes );
    bool SetNonblockingRecv( int timeoutMilliseconds );
};

class ServerSocket
{
private:
    struct addrinfo* m_addr = nullptr;
    SOCKET m_listenSocket   = INVALID_SOCKET;

public:
    ServerSocket() = default;
    ~ServerSocket();

    bool Open( const char* host, const int port, int clientQueueSize = 16 );
    bool AcceptConnection( ClientSocket& clientSocket );
    bool Close();
};

bool InitSocketsLib();
void ShutdownSocketLib();
