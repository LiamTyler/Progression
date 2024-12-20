#include "shared/serializer.hpp"
#include "shared/assert.hpp"
#include "shared/filesystem.hpp"
#include "shared/logger.hpp"
#include <cstring>

Serializer::~Serializer() { Close(); }

bool Serializer::OpenForRead( const std::string& fname )
{
    PG_DBG_ASSERT( !IsOpen(), "Dont forget to close last file used" );
    filename = fname;
    if ( !memMappedFile.open( fname, MemoryMapped::WholeFile, MemoryMapped::SequentialScan ) )
    {
        return false;
    }
    currentReadPos = memMappedFile.getData();

    return true;
}

bool Serializer::OpenForWrite( const std::string& fname )
{
    PG_DBG_ASSERT( !IsOpen(), "Dont forget to close last file used" );
    filename               = fname;
    std::string parentPath = GetParentPath( fname );
    if ( !parentPath.empty() && !PathExists( parentPath ) )
    {
        CreateDirectory( parentPath );
    }
    writeFile.open( fname, std::ios::binary );
    if ( !writeFile || !writeFile.is_open() )
    {
        LOG_ERR( "Could not open file '%s'", fname.c_str() );
        return false;
    }

    return true;
}

void Serializer::Close()
{
    if ( memMappedFile.isValid() )
    {
        memMappedFile.close();
        currentReadPos = nullptr;
    }
    else if ( writeFile.is_open() )
    {
        writeFile.close();
    }
}

bool Serializer::IsOpen() const { return writeFile.is_open() || memMappedFile.isValid(); }

size_t Serializer::BytesLeft() const
{
    if ( memMappedFile.isValid() )
    {
        size_t bytesRead = currentReadPos - memMappedFile.getData();
        return memMappedFile.size() - bytesRead;
    }

    return 0;
}

const u8* Serializer::GetData() const
{
    PG_DBG_ASSERT( memMappedFile.isValid() );
    return currentReadPos;
}

void Serializer::Write( const void* buffer, size_t bytes )
{
    PG_DBG_ASSERT( writeFile.good() && ( buffer || ( !buffer && !bytes ) ) );
    writeFile.write( reinterpret_cast<const char*>( buffer ), bytes );
}

void Serializer::Read( void* buffer, size_t bytes )
{
    PG_DBG_ASSERT( !bytes || ( buffer && currentReadPos ) );
    PG_DBG_ASSERT(
        !bytes || ( currentReadPos - memMappedFile.getData() + bytes <= memMappedFile.size() ), "Reading off the end of the file" );
    memcpy( buffer, currentReadPos, bytes );
    currentReadPos += bytes;
}

void Serializer::Skip( size_t bytes )
{
    PG_DBG_ASSERT(
        !bytes || ( currentReadPos - memMappedFile.getData() + bytes <= memMappedFile.size() ), "Skipping off the end of the file" );
    currentReadPos += bytes;
}
