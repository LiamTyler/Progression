#include "sockets.hpp"
#include "logger.hpp"

ClientSocket::~ClientSocket() { Close(); }

bool ClientSocket::OpenSocket( const char* host, const int port )
{
    struct addrinfo hints;
    ZeroMemory( &hints, sizeof( hints ) );
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    char portStr[32];
    sprintf_s( portStr, "%d", port );

    m_addr      = NULL;
    int iResult = getaddrinfo( host, portStr, &hints, &m_addr );
    if ( iResult != 0 )
    {
        LOG_ERR( "getaddrinfo failed with error: %d", iResult );
        return false;
    }

    m_connectSocket = socket( m_addr->ai_family, m_addr->ai_socktype, m_addr->ai_protocol );
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
        m_connectSocket = INVALID_SOCKET;
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

int ClientSocket::ReceiveData( void* buffer, int bufferSizeInBytes )
{
    return recv( m_connectSocket, (char*)buffer, bufferSizeInBytes, 0 );
}

bool ClientSocket::SetNonblockingRecv( int timeoutMilliseconds )
{
    timeval timeout;
    timeout.tv_sec  = timeoutMilliseconds;
    timeout.tv_usec = 0;
    return setsockopt( m_connectSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof( timeout ) ) != SOCKET_ERROR;
}

ServerSocket::~ServerSocket() { Close(); }

bool ServerSocket::Open( const char* host, const int port, int clientQueueSize )
{
    struct addrinfo hints;
    ZeroMemory( &hints, sizeof( hints ) );
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags    = AI_PASSIVE;
    char portStr[32];
    sprintf_s( portStr, "%d", port );

    m_addr      = NULL;
    int iResult = getaddrinfo( NULL, portStr, &hints, &m_addr );
    if ( iResult != 0 )
    {
        LOG_ERR( "getaddrinfo failed with error: %d", iResult );
        return false;
    }

    m_listenSocket = socket( m_addr->ai_family, m_addr->ai_socktype, m_addr->ai_protocol );
    if ( m_listenSocket == INVALID_SOCKET )
    {
#if USING( WINDOWS_PROGRAM )
        LOG_ERR( "Failed to create socket with error: %ld", WSAGetLastError() );
#else  // #if USING( WINDOWS_PROGRAM )
        LOG_ERR( "Failed to create socket" );
#endif // #else // #if USING( WINDOWS_PROGRAM )
        return false;
    }

    iResult = bind( m_listenSocket, m_addr->ai_addr, (int)m_addr->ai_addrlen );
    if ( iResult == SOCKET_ERROR )
    {
#if USING( WINDOWS_PROGRAM )
        LOG_ERR( "bind failed with error: %d", WSAGetLastError() );
#else  // #if USING( WINDOWS_PROGRAM )
        LOG_ERR( "bind failed" );
#endif // #else // #if USING( WINDOWS_PROGRAM )
        return false;
    }

    iResult = listen( m_listenSocket, clientQueueSize );
    if ( iResult == SOCKET_ERROR )
    {
#if USING( WINDOWS_PROGRAM )
        LOG_ERR( "listen failed with error: %d", WSAGetLastError() );
#else
        LOG_ERR( "listen failed" );
#endif // #else // #if USING( WINDOWS_PROGRAM )
        return false;
    }

    return true;
}

bool ServerSocket::AcceptConnection( ClientSocket& clientSocket )
{
    clientSocket.m_connectSocket = accept( m_listenSocket, NULL, NULL );
    if ( clientSocket.m_connectSocket == INVALID_SOCKET )
    {
#if USING( WINDOWS_PROGRAM )
        LOG_ERR( "AcceptConnection failed with error: %d", WSAGetLastError() );
#else  // #if USING( WINDOWS_PROGRAM )
        LOG_ERR( "AcceptConnection failed" );
#endif // #else // #if USING( WINDOWS_PROGRAM )
        return false;
    }

    return true;
}

bool ServerSocket::Close()
{
    bool success = true;
    if ( m_listenSocket != INVALID_SOCKET )
    {
        int iResult = closesocket( m_listenSocket );
        if ( iResult == SOCKET_ERROR )
        {
#if USING( WINDOWS_PROGRAM )
            LOG_ERR( "ServerSocket closesocket failed with error: %d", WSAGetLastError() );
#else  // #if USING( WINDOWS_PROGRAM )
            LOG_ERR( "ServerSocket closesocket failed" );
#endif // #else // #if USING( WINDOWS_PROGRAM )
            success = false;
        }
        m_listenSocket = INVALID_SOCKET;
    }

    return success;
}

#if USING( WINDOWS_PROGRAM )
static bool s_wsaInit = false;
#endif // #if USING( WINDOWS_PROGRAM )

bool InitSocketsLib()
{
#if USING( WINDOWS_PROGRAM )
    if ( s_wsaInit )
        return true;

    WSADATA wsaData;
    int iResult = WSAStartup( MAKEWORD( 2, 2 ), &wsaData );
    if ( iResult != 0 )
    {
        LOG_ERR( "WSAStartup failed with error: %d\n", iResult );
        return false;
    }
    s_wsaInit = true;
#endif // #if USING( WINDOWS_PROGRAM )

    return true;
}

void ShutdownSocketLib()
{
#if USING( WINDOWS_PROGRAM )
    if ( s_wsaInit )
    {
        s_wsaInit = false;
        WSACleanup();
    }
#endif // #if USING( WINDOWS_PROGRAM )
}
