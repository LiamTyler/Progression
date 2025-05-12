#include "sockets.hpp"
#include "logger.hpp"

ClientSocket::~ClientSocket() { Close(); }

bool ClientSocket::OpenSocket( const char* port )
{
    struct addrinfo hints;
    ZeroMemory( &hints, sizeof( hints ) );
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    m_addr      = NULL;
    int iResult = getaddrinfo( NULL, port, &hints, &m_addr );
    if ( iResult != 0 )
    {
        LOG_ERR( "getaddrinfo failed with error: %d", iResult );
        return false;
    }

    m_connectSocket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    if ( m_connectSocket == INVALID_SOCKET )
    {
#if USING( WINDOWS_PROGRAM )
        LOG_ERR( "Failed to create socket with error: %ld", WSAGetLastError() );
#else  // #if USING( WINDOWS_PROGRAM )
        LOG_ERR( "Failed to create socket" );
#endif // #else // #if USING( WINDOWS_PROGRAM )
        return false;
    }

    return true;
}

bool ClientSocket::OpenConnection()
{
    int iResult = connect( m_connectSocket, m_addr->ai_addr, (int)m_addr->ai_addrlen );
    if ( iResult == SOCKET_ERROR || m_connectSocket == INVALID_SOCKET )
    {
        LOG_ERR( "Unable to connect socket" );
        return false;
    }

    return true;
}

bool ClientSocket::Close()
{
    bool success = true;
    if ( m_connectSocket != INVALID_SOCKET )
    {
        int iResult = shutdown( m_connectSocket, SD_SEND );
        if ( iResult == SOCKET_ERROR )
        {
#if USING( WINDOWS_PROGRAM )
            LOG_ERR( "ClientSocket shutdown failed with error: %d", WSAGetLastError() );
#else  // #if USING( WINDOWS_PROGRAM )
            LOG_ERR( "ClientSocket shutdown failed" );
#endif // #else // #if USING( WINDOWS_PROGRAM )
            success = false;
        }

        iResult = closesocket( m_connectSocket );
        if ( iResult == SOCKET_ERROR )
        {
#if USING( WINDOWS_PROGRAM )
            LOG_ERR( "ClientSocket closesocket failed with error: %d", WSAGetLastError() );
#else  // #if USING( WINDOWS_PROGRAM )
            LOG_ERR( "ClientSocket closesocket failed" );
#endif // #else // #if USING( WINDOWS_PROGRAM )
            success = false;
        }
    }
    if ( m_addr )
    {
        freeaddrinfo( m_addr );
        m_addr = nullptr;
    }

    return success;
}

bool ClientSocket::SendData( const void* data, int sizeInBytes )
{
#if USING( WINDOWS_PROGRAM )
    int iResult = send( m_connectSocket, (const char*)data, sizeInBytes, 0 );
    if ( iResult == SOCKET_ERROR )
    {
        int err = WSAGetLastError();
        if ( err == WSAECONNRESET )
            LOG_ERR( "ClientSocket::SendData error: the server was closed! Closing" );
        else
            LOG_ERR( "Failed to send command to server with error: %d. Closing", WSAGetLastError() );
        Close();
        return false;
    }
    else if ( iResult != sizeInBytes )
    {
        LOG_ERR( "ClientSocket::SendData: only sent %d bytes", iResult );
        return false;
    }
#else  // #if USING( WINDOWS_PROGRAM )
    return false;
#endif // #else // #if USING( WINDOWS_PROGRAM )
    return true;
}

bool ClientSocket::ReceiveData( void* buffer, int sizeInBytes )
{
#if USING( WINDOWS_PROGRAM )
    char* buf     = (char*)buffer;
    int recvBytes = 0;
    do
    {
        int iResult = recv( m_connectSocket, buf + recvBytes, sizeInBytes - recvBytes, 0 );
        if ( iResult == 0 )
        {
            LOG_ERR( "ClientSocket::ReceiveData: connection closed. Closing" );
            Close();
            return false;
        }
        else if ( iResult < 0 )
        {
            LOG_ERR( "ClientSocket::ReceiveData: recv failed with error: %d. Closing", WSAGetLastError() );
            Close();
            return false;
        }
        recvBytes += sizeInBytes;
    } while ( recvBytes != sizeInBytes );
#else  // #if USING( WINDOWS_PROGRAM )
    return false;
#endif // #else // #if USING( WINDOWS_PROGRAM )
    return true;
}

bool InitSocketsLib()
{
#if USING( WINDOWS_PROGRAM )
    WSADATA wsaData;
    int iResult = WSAStartup( MAKEWORD( 2, 2 ), &wsaData );
    if ( iResult != 0 )
    {
        LOG_ERR( "WSAStartup failed with error: %d\n", iResult );
        return false;
    }
#endif // #if USING( WINDOWS_PROGRAM )

    return true;
}

void ShutdownSocketLib()
{
#if USING( WINDOWS_PROGRAM )
    WSACleanup();
#endif // #if USING( WINDOWS_PROGRAM )
}
