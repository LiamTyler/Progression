#pragma once

#include <functional>
#include <string>

template <class T>
inline void HashCombine( std::size_t& seed, const T& v )
{
    std::hash<T> hasher;
    seed ^= hasher( v ) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}


inline void HashCombine( std::size_t& seed, const char* str )
{
    std::string s( str );
    std::hash<std::string> hasher;
    seed ^= hasher( s ) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}


inline size_t Hash( const std::string& str )
{
    return std::hash< std::string >{}( str );
}