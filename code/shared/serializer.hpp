#pragma once

#include "memory_map/MemoryMapped.h"
#include "shared/core_defines.hpp"
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

class Serializer
{
public:
    Serializer() = default;
    ~Serializer();

    bool OpenForRead( const std::string& filename );
    bool OpenForWrite( const std::string& filename );
    void Close();
    bool IsOpen() const;

    // if opened for reading, return how many bytes are left to read. Else 0
    size_t BytesLeft() const;
    const u8* GetData() const;

    void Write( const void* buffer, size_t bytes );
    void Read( void* buffer, size_t bytes );
    void Skip( size_t bytes );

    template <typename LenType = u32>
    void Write( const std::string& s )
    {
        LenType strSize = static_cast<LenType>( s.length() );
        Write( strSize );
        if ( strSize > 0 )
        {
            Write( &s[0], strSize );
        }
    }

    template <typename LenType = i32>
    void Read( std::string& s )
    {
        LenType strSize;
        Read( strSize );
        if ( strSize > 0 )
        {
            s.resize( strSize );
            Read( &s[0], strSize );
        }
    }

    template <typename T>
    void Write( const T& x )
    {
        static_assert( std::is_trivial<T>::value, "T must be a trivial plain old data type" );
        Write( &x, sizeof( T ) );
    }

    template <typename T>
    void Write( const std::vector<T>& x )
    {
        static_assert( std::is_trivial<T>::value, "T must be a trivial plain old data type" );
        size_t size = x.size();
        Write( size );
        Write( x.data(), x.size() * sizeof( T ) );
    }

    template <typename T>
    void Write( const char* str )
    {
        T len = (T)strlen( str );
        Write( len );
        Write( str, len );
    }

    template <typename T>
    void Read( T& x )
    {
        static_assert( !std::is_const<T>::value, "T must be non-const" );
        static_assert( std::is_trivial<T>::value, "T must be a trivial plain old data type" );
        Read( (void*)&x, sizeof( T ) );
    }

    template <typename T>
    void Read( std::vector<T>& x )
    {
        static_assert( !std::is_const<T>::value, "T must be non-const" );
        static_assert( std::is_trivial<T>::value, "T must be a trivial plain old data type" );
        size_t size;
        Read( size );
        x.resize( size );
        Read( (void*)x.data(), size * sizeof( T ) );
    }

    template <typename T>
    T Read()
    {
        T val;
        Read( val );
        return val;
    }

private:
    std::string filename;
    std::ofstream writeFile;
    MemoryMapped memMappedFile;
    unsigned char* currentReadPos = nullptr;
};
