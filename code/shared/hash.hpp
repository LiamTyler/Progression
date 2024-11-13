#pragma once

#include "shared/math_vec.hpp"
#include <functional>
#include <string>

template <class T>
inline void HashCombine( std::size_t& seed, const T& v )
{
    std::hash<T> hasher;
    seed ^= hasher( v ) + 0x9e3779b9 + ( seed << 6 ) + ( seed >> 2 );
}

inline void HashCombine( std::size_t& seed, const char* str )
{
    std::string s( str );
    std::hash<std::string> hasher;
    seed ^= hasher( s ) + 0x9e3779b9 + ( seed << 6 ) + ( seed >> 2 );
}

inline void HashCombine( std::size_t& seed, vec2 v )
{
    HashCombine( seed, v.x );
    HashCombine( seed, v.y );
}

inline void HashCombine( std::size_t& seed, vec3 v )
{
    HashCombine( seed, v.x );
    HashCombine( seed, v.y );
    HashCombine( seed, v.z );
}

inline void HashCombine( std::size_t& seed, vec4 v )
{
    HashCombine( seed, v.x );
    HashCombine( seed, v.y );
    HashCombine( seed, v.z );
    HashCombine( seed, v.w );
}

template <class T>
inline size_t Hash( const T& x )
{
    return std::hash<T>{}( x );
}

inline size_t Hash( const vec2& v )
{
    size_t seed = 0;
    HashCombine( seed, v.x );
    HashCombine( seed, v.y );
    return seed;
}

inline size_t Hash( const vec3& v )
{
    size_t seed = 0;
    HashCombine( seed, v.x );
    HashCombine( seed, v.y );
    HashCombine( seed, v.z );
    return seed;
}

inline size_t Hash( const vec4& v )
{
    size_t seed = 0;
    HashCombine( seed, v.x );
    HashCombine( seed, v.y );
    HashCombine( seed, v.z );
    HashCombine( seed, v.w );
    return seed;
}
