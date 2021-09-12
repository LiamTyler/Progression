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
#define PG_NO_WARN_UNUSED( x ) (void) ( x );

// https://stackoverflow.com/questions/2124339/c-preprocessor-va-args-number-of-arguments
#define PG_EXPAND( x ) x
#define PG_ARG_N( _1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,N,...) N
#define PG_NARG( ... ) PG_EXPAND( PG_ARG_N( __VA_ARGS__, 20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0 ) )