#pragma once

#ifdef _MSC_VER
#include <intrin.h>
#endif

inline uint32_t clz( uint32_t x )
{
#ifdef __GNUC__
		return ((x) == 0 ? 32 : __builtin_clz( x ));
#elif defined( _MSC_VER )
	unsigned long result;
	if ( _BitScanReverse( &result, x ) )
	{
		return 31 - result;
	}
	else
	{
		return 32;
	}
#else
	#error "Bitop clz not implemented yet for this platform"
#endif
}


inline uint32_t ctz( uint32_t x )
{
#ifdef __GNUC__
		return ((x) == 0 ? 32 : __builtin_ctz( x ));
#elif defined( _MSC_VER )
	unsigned long result;
	if ( _BitScanForward( &result, x ) )
	{
		return result;
	}
	else
	{
		return 32;
	}
#else
	#error "Bitop clz not implemented yet for this platform"
#endif
}


template <typename T>
inline void ForEachBit( uint32_t value, const T& func )
{
	while ( value )
	{
		uint32_t bit = ctz( value );
		func( bit );
		value &= ~(1u << bit);
	}
}