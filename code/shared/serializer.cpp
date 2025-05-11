#include "shared/serializer.hpp"
#include "shared/assert.hpp"
#include "shared/filesystem.hpp"
#include "shared/logger.hpp"
#include <cstring>

static constexpr size_t SCRATCH_BUFFER_SIZE = 32 * 1024 * 1024;

Serializer::~Serializer() { Close(); }

bool Serializer::OpenForRead( const std::string& fname )
{
    PG_DBG_ASSERT( !IsOpen(), "Dont forget to close last file used" );
    m_filename = fname;
    if ( !m_memMappedFile.open( fname, MemoryMapped::WholeFile, MemoryMapped::SequentialScan ) )
    {
        return false;
    }
    m_currentReadPos = m_memMappedFile.getData();

    return true;
}

bool Serializer::OpenForWrite( const std::string& fname )
{
    PG_DBG_ASSERT( !IsOpen(), "Dont forget to close last file used" );
    m_filename             = fname;
    std::string parentPath = GetParentPath( fname );
    if ( !parentPath.empty() && !PathExists( parentPath ) )
    {
        CreateDirectory( parentPath );
    }
    m_writeFile.open( fname, std::ios::binary );
    if ( !m_writeFile || !m_writeFile.is_open() )
    {
        LOG_ERR( "Could not open file '%s'", fname.c_str() );
        return false;
    }

    m_bytesWritten  = 0;
    m_scratchBuffer = (char*)malloc( SCRATCH_BUFFER_SIZE );
    m_scratchPos    = 0;

    return true;
}

size_t Serializer::Close()
{
    if ( m_memMappedFile.isValid() )
    {
        m_memMappedFile.close();
        m_currentReadPos = nullptr;
    }
    else if ( m_writeFile.is_open() )
    {
        Flush();
        free( m_scratchBuffer );
        m_writeFile.close();
        return m_bytesWritten;
    }

    return 0;
}

void Serializer::Flush()
{
    if ( m_scratchPos )
    {
        if ( m_flushCallback )
            m_flushCallback( m_scratchBuffer, m_scratchPos, m_userdata );

        m_writeFile.write( m_scratchBuffer, m_scratchPos );
        m_bytesWritten += m_scratchPos;
        m_scratchPos = 0;
    }
}

bool Serializer::IsOpen() const { return m_writeFile.is_open() || m_memMappedFile.isValid(); }

size_t Serializer::BytesLeft() const
{
    if ( m_memMappedFile.isValid() )
    {
        size_t bytesRead = m_currentReadPos - m_memMappedFile.getData();
        return m_memMappedFile.size() - bytesRead;
    }

    return 0;
}

const u8* Serializer::GetData() const
{
    PG_DBG_ASSERT( m_memMappedFile.isValid() );
    return m_currentReadPos;
}

void Serializer::Write( const void* buffer, size_t bytes )
{
    if ( !buffer || !bytes )
        return;

    // m_writeFile.write( (const char*)buffer, bytes );
    // return;

    const char* inputBuff = (const char*)buffer;
    while ( bytes )
    {
        size_t toWrite = std::min( bytes, SCRATCH_BUFFER_SIZE - m_scratchPos );
        memcpy( m_scratchBuffer + m_scratchPos, inputBuff, toWrite );
        m_scratchPos += toWrite;
        inputBuff += toWrite;
        bytes -= toWrite;
        if ( m_scratchPos == SCRATCH_BUFFER_SIZE )
            Flush();
    }
}

void Serializer::Read( void* buffer, size_t bytes )
{
    PG_DBG_ASSERT( !bytes || ( buffer && m_currentReadPos ) );
    PG_DBG_ASSERT(
        !bytes || ( m_currentReadPos - m_memMappedFile.getData() + bytes <= m_memMappedFile.size() ), "Reading off the end of the file" );
    memcpy( buffer, m_currentReadPos, bytes );
    m_currentReadPos += bytes;
}

void Serializer::Skip( size_t bytes )
{
    PG_DBG_ASSERT(
        !bytes || ( m_currentReadPos - m_memMappedFile.getData() + bytes <= m_memMappedFile.size() ), "Skipping off the end of the file" );
    m_currentReadPos += bytes;
}
