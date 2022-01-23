#include "shared/serializer.hpp"
#include "shared/assert.hpp"
#include "shared/filesystem.hpp"
#include "shared/logger.hpp"
#include <cstring>

Serializer::~Serializer()
{
    Close();
}


bool Serializer::OpenForRead( const std::string& fname )
{
    PG_ASSERT( !IsOpen(), "Dont forget to close last file used" );
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
    PG_ASSERT( !IsOpen(), "Dont forget to close last file used" );
    filename = fname;
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


bool Serializer::IsOpen() const
{
    return writeFile.is_open() || memMappedFile.isValid();
}


size_t Serializer::BytesLeft() const
{
    if ( memMappedFile.isValid() )
    {
        size_t bytesRead = currentReadPos - memMappedFile.getData();
        return memMappedFile.size() - bytesRead;
    }

    return 0;
}


void Serializer::Write( const void* buffer, size_t bytes )
{
    PG_ASSERT( writeFile.good() && (buffer || (!buffer && !bytes)) );
    writeFile.write( reinterpret_cast< const char* >( buffer ), bytes );
}


void Serializer::Read( void* buffer, size_t bytes )
{
    PG_ASSERT( !bytes || (buffer && currentReadPos) );
    PG_ASSERT( !bytes || (currentReadPos - memMappedFile.getData() + bytes <= memMappedFile.size()), "Reading off the end of the file" );
    memcpy( buffer, currentReadPos, bytes );
    currentReadPos += bytes;
}


void Serializer::Write( const std::string& s )
{
    int strSize = static_cast< int >( s.length() );
    Write( strSize );
    if ( strSize > 0 )
    {
        Write( &s[0], strSize );
    }
}


void Serializer::Read( std::string& s )
{
    int strSize;
    Read( strSize );
    if ( strSize > 0 )
    {
        s.resize( strSize, ' ' );
        Read( &s[0], strSize );
    }
}
