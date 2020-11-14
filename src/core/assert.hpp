#pragma once

#include "platform_defines.hpp"
#include <cstdio>
#include <cstdlib>


#define _PG_ASSERT_NO_MSG( x )                                                           \
if ( !( x ) )                                                                            \
{                                                                                        \
    printf( "Failed assertion: (%s) at line %d in file %s.\n", #x, __LINE__, __FILE__ ); \
    fflush( stdout );                                                                    \
    abort();                                                                             \
}

#define _PG_ASSERT_WITH_MSG( x, msg )                                                                                   \
if ( !( x ) )                                                                                                           \
{                                                                                                                       \
    printf( "Failed assertion: (%s) at line %d in file %s: %s\n", #x, __LINE__, __FILE__, std::string( msg ).c_str() ); \
    fflush( stdout );                                                                                                   \
    abort();                                                                                                            \
}

#define _PG_GET_ASSERT_MACRO( _1, _2, NAME, ... ) NAME

#if !USING( SHIP_BUILD )
#include <string>

#if !USING( WINDOWS_PROGRAM )
#define PG_ASSERT( ... ) _PG_GET_ASSERT_MACRO( __VA_ARGS__, _PG_ASSERT_WITH_MSG, _PG_ASSERT_NO_MSG )( __VA_ARGS__ )
#else // #if !USING( WINDOWS_PROGRAM )
#define _PG_EXPAND( x ) x
#define PG_ASSERT( ... ) _PG_EXPAND( _PG_GET_ASSERT_MACRO( __VA_ARGS__, _PG_ASSERT_WITH_MSG, _PG_ASSERT_NO_MSG )( __VA_ARGS__ ) )
#endif // #else // #if !USING( WINDOWS_PROGRAM )

#else // #if !USING( SHIP_BUILD )

#define PG_ASSERT( ... ) do {} while ( 0 )

#endif // #else // #if !USING( SHIP_BUILD )

// Some things, like std::vector/string can have slightly different sizes in debug vs release mode
#if !USING( DEBUG_BUILD )
#define PG_STATIC_NDEBUG_ASSERT( test, msg ) static_assert( test, msg )
#else // #if !USING( DEBUG_BUILD )
#define PG_STATIC_NDEBUG_ASSERT( test, msg ) do {} while ( 0 )
#endif // #else // #if !USING( DEBUG_BUILD )