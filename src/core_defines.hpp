#pragma once

/*
 * With normal #if and #ifdef its easy to accidentally use an undefined / wrong symbol.
 * This will catch when that happens, usually with a div by zero compile error.
 */
#define IN_USE 9
#define NOT_IN_USE ( -9 )
#define USING(x) ( 9 / ( x ) == 1 )

#define ARRAY_COUNT( array ) ( static_cast< int >( sizeof( array ) / sizeof( array[0] ) ) )
#define PG_UNUSED( x ) (void) ( x );
#define PG_MAYBE_UNUSED( x ) (void) ( x );
