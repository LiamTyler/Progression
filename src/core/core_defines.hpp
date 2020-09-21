#pragma once

/*
 * With a normal #if, accidentally using wrong symbol will not give error, but just get interpreted as 0.
 * With a normal #ifdef, accidentally using the wrong symbol just skips the inteded path with no error.
 * This will helps catch those instances, since using the wrong symbol will give a divide by zero compile error.
 */
#define IN_USE 9
#define NOT_IN_USE ( -9 )
#define USING( x ) ( 9 / ( x ) == 1 )

#define ARRAY_COUNT( array ) ( static_cast< int >( sizeof( array ) / sizeof( array[0] ) ) )
#define PG_UNUSED( x ) (void) ( x );
#define PG_MAYBE_UNUSED( x ) (void) ( x );
