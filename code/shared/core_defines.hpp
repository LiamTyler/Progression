#pragma once

#include <stddef.h>
#include <type_traits>

/*
 * With a normal #if, accidentally using wrong symbol will not give error, but just get interpreted as 0.
 * With a normal #ifdef, accidentally using the wrong symbol just skips the inteded path with no error.
 * This will helps catch those instances, since using the wrong symbol will give a divide by zero compile error.
 */
#define IN_USE &&
#define NOT_IN_USE &&!
#define USING( x ) ( 1 x 1 )
#define USE_IF( x ) &&( ( x ) ? 1 : 0 )&&

#define ARRAY_COUNT( array ) ( static_cast<int>( sizeof( array ) / sizeof( array[0] ) ) )
#define PG_UNUSED( x ) (void)( x );
#define PG_NO_WARN_UNUSED( x ) (void)( x );

// clang-format off
// requires c++20 for __VA_OPT__
#define _VA_SIZE_HELPER( _1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32, COUNT, ... ) COUNT
#define VA_SIZE( ... ) _VA_SIZE_HELPER(__VA_ARGS__ __VA_OPT__(,) 32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0)

#define _PG_CAT( A, B ) A ## B
#define _PG_SELECT( NAME, NUM ) _PG_CAT( NAME, NUM )
#define _PG_VA_SELECT( NAME, ... ) _PG_SELECT( NAME, VA_SIZE(__VA_ARGS__)) (__VA_ARGS__)
// clang-format on

// Only if alignment is a power of 2
#define ALIGN_UP_POW_2( val, alignment ) ( ( val + alignment - 1 ) & ~( alignment - 1 ) )

// https://stackoverflow.com/questions/36568050/sfinae-not-happening-with-stdunderlying-type
// since std::underlying_type is undefined for non-enums
template <typename T, bool = std::is_enum<T>::value>
struct relaxed_underlying_type
{
    using type = typename std::underlying_type<T>::type;
};

template <typename T>
struct relaxed_underlying_type<T, false>
{
    using type = T;
};

template <typename T>
constexpr relaxed_underlying_type<T>::type Underlying( T val )
{
    return static_cast<relaxed_underlying_type<T>::type>( val );
}

template <typename T, size_t SIZE>
constexpr T ARRAY_SUM( const T ( &arr )[SIZE] )
{
    T sum = 0;
    for ( size_t i = 0; i < SIZE; ++i )
        sum += arr[i];

    return sum;
}

#define PG_DEFINE_ENUM_OPS( T )                                                                              \
    constexpr inline T operator~( T a ) { return static_cast<T>( ~Underlying( a ) ); }                       \
    constexpr inline T operator|( T a, T b ) { return static_cast<T>( Underlying( a ) | Underlying( b ) ); } \
    constexpr inline T operator&( T a, T b ) { return static_cast<T>( Underlying( a ) & Underlying( b ) ); } \
    constexpr inline T operator^( T a, T b ) { return static_cast<T>( Underlying( a ) ^ Underlying( b ) ); } \
    inline T& operator|=( T& a, T b )                                                                        \
    {                                                                                                        \
        return reinterpret_cast<T&>( reinterpret_cast<std::underlying_type_t<T>&>( a ) |= Underlying( b ) ); \
    }                                                                                                        \
    inline T& operator&=( T& a, T b )                                                                        \
    {                                                                                                        \
        return reinterpret_cast<T&>( reinterpret_cast<std::underlying_type_t<T>&>( a ) &= Underlying( b ) ); \
    }                                                                                                        \
    inline T& operator^=( T& a, T b )                                                                        \
    {                                                                                                        \
        return reinterpret_cast<T&>( reinterpret_cast<std::underlying_type_t<T>&>( a ) ^= Underlying( b ) ); \
    }                                                                                                        \
    constexpr bool IsSet( T a, T b ) { return ( a & b ) == b; }
