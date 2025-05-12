#pragma once

#include "platform_defines.hpp"

#if USING( WINDOWS_PROGRAM )
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment( lib, "Ws2_32.lib" )
#endif // #if USING( WINDOWS_PROGRAM )

class ClientSocket
{
private:
    struct addrinfo* m_addr = nullptr;
    SOCKET m_connectSocket  = INVALID_SOCKET;

public:
    ClientSocket() = default;
    ~ClientSocket();

    bool OpenSocket( const char* port );
    bool OpenConnection();
    bool Close();

    bool SendData( const void* data, int sizeInBytes );
    bool ReceiveData( void* buffer, int bufferSizeInBytes );
};

bool InitSocketsLib();
void ShutdownSocketLib();
