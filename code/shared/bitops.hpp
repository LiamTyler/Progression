#pragma once

#ifdef _MSC_VER
#include <intrin.h>
#endif

inline unsigned long clz( unsigned long x )
{
    if ( !x )
        return 32;
#ifdef __GNUC__
    return __builtin_clzl( x );
#elif defined( _MSC_VER )
    unsigned long result;
    _BitScanReverse( &result, x );
    return 31 - result;
#else
#error "Bitop clzl not implemented yet for this platform"
#endif
}

inline unsigned long clzll( unsigned long long x )
{
    if ( !x )
        return 64;
#ifdef __GNUC__
    return __builtin_clzll( x );
#elif defined( _MSC_VER )
    unsigned long result;
    _BitScanReverse64( &result, x );
    return 63 - result;
#else
#error "Bitop clzll not implemented yet for this platform"
#endif
}

inline unsigned long ctz( unsigned long x )
{
    if ( !x )
        return 32;
#ifdef __GNUC__
    return __builtin_ctzl( x );
#elif defined( _MSC_VER )
    unsigned long result;
    _BitScanForward( &result, x );
    return result;
#else
#error "Bitop ctzl not implemented yet for this platform"
#endif
}

inline unsigned long ctzll( unsigned long long x )
{
    if ( !x )
        return 64;
#ifdef __GNUC__
    return __builtin_ctz( x );
#elif defined( _MSC_VER )
    unsigned long result;
    _BitScanForward64( &result, x );
    return result;
#else
#error "Bitop ctzll not implemented yet for this platform"
#endif
}

inline unsigned long ctz_nonzero( unsigned long x )
{
#ifdef __GNUC__
    return __builtin_ctz( x );
#elif defined( _MSC_VER )
    unsigned long result;
    _BitScanForward( &result, x );
    return result;
#else
#error "Bitop ctz not implemented yet for this platform"
#endif
}

inline unsigned long ctzll_nonzero( unsigned long long x )
{
#ifdef __GNUC__
    return __builtin_ctzll( x );
#elif defined( _MSC_VER )
    unsigned long result;
    _BitScanForward64( &result, x );
    return result;
#else
#error "Bitop ctzll not implemented yet for this platform"
#endif
}

template <typename T>
inline void ForEachBit32( unsigned long value, const T& func )
{
    while ( value )
    {
        unsigned long bit = ctz_nonzero( value );
        func( bit );
        value &= ~( 1u << bit );
    }
}

template <typename T>
inline void ForEachBit64( unsigned long long value, const T& func )
{
    while ( value )
    {
        unsigned long bit = ctzll_nonzero( value );
        func( bit );
        value &= ~( 1u << bit );
    }
}
