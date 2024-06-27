#pragma once

#include "platform_defines.hpp"
#include "shared/logger.hpp"
#include <cstdio>
#include <cstdlib>

#if USING( DEVELOPMENT_BUILD )

#define PG_ASSERT( ... ) _PG_VA_SELECT( _PG_ASSERT, __VA_ARGS__ )

#if USING( DEBUG_BUILD )
#define PG_DBG_ASSERT( ... ) _PG_VA_SELECT( _PG_ASSERT, __VA_ARGS__ )
#else // #if USING( DEBUG_BUILD )
#define PG_DBG_ASSERT( ... ) \
    do                       \
    {                        \
    } while ( 0 )
#endif // #else // #if USING( DEBUG_BUILD )

#define _PG_ASSERT_START( X )                                                             \
    if ( !( X ) )                                                                         \
    {                                                                                     \
        Logger_Log( LogSeverity::ERR, TerminalColorCode::RED, TerminalEmphasisCode::NONE, \
            "Failed assertion: (%s) at line %d in file %s\n", #X, __LINE__, __FILE__ );

#define _PG_ASSERT_END() \
    abort();             \
    }

#define _PG_FMG_CHK1( FMT, A1 ) constexpr FormatCheckError _pgFmtError = CheckFormat<_PG_DWRAP1( A1 )>( std::string_view( FMT ) )
#define _PG_FMG_CHK2( FMT, A1, A2 ) constexpr FormatCheckError _pgFmtError = CheckFormat<_PG_DWRAP2( A1, A2 )>( std::string_view( FMT ) )
#define _PG_FMG_CHK3( FMT, A1, A2, A3 ) \
    constexpr FormatCheckError _pgFmtError = CheckFormat<_PG_DWRAP3( A1, A2, A3 )>( std::string_view( FMT ) )
#define _PG_FMG_CHK4( FMT, A1, A2, A3, A4 ) \
    constexpr FormatCheckError _pgFmtError = CheckFormat<_PG_DWRAP4( A1, A2, A3, A4 )>( std::string_view( FMT ) )
#define _PG_FMG_CHK5( FMT, A1, A2, A3, A4, A5 ) \
    constexpr FormatCheckError _pgFmtError = CheckFormat<_PG_DWRAP5( A1, A2, A3, A4, A5 )>( std::string_view( FMT ) )
#define _PG_FMG_CHK6( FMT, A1, A2, A3, A4, A5, A6 ) \
    constexpr FormatCheckError _pgFmtError = CheckFormat<_PG_DWRAP6( A1, A2, A3, A4, A5, A6 )>( std::string_view( FMT ) )
#define _PG_FMG_CHK7( FMT, A1, A2, A3, A4, A5, A6, A7 ) \
    constexpr FormatCheckError _pgFmtError = CheckFormat<_PG_DWRAP7( A1, A2, A3, A4, A5, A6, A7 )>( std::string_view( FMT ) )
#define _PG_FMG_CHK8( FMT, A1, A2, A3, A4, A5, A6, A7, A8 ) \
    constexpr FormatCheckError _pgFmtError = CheckFormat<_PG_DWRAP8( A1, A2, A3, A4, A5, A6, A7, A8 )>( std::string_view( FMT ) )
#define _PG_FMG_CHK9( FMT, A1, A2, A3, A4, A5, A6, A7, A8, A9 ) \
    constexpr FormatCheckError _pgFmtError = CheckFormat<_PG_DWRAP9( A1, A2, A3, A4, A5, A6, A7, A8, A9 )>( std::string_view( FMT ) )
#define _PG_FMG_CHK10( FMT, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10 ) \
    constexpr FormatCheckError _pgFmtError = CheckFormat<_PG_DWRAP10( A1, A2, A3, A4, A5, A6, A7, A8, A9, A10 )>( std::string_view( FMT ) )

#define _PG_ASSERT1( X )  \
    _PG_ASSERT_START( X ) \
    _PG_ASSERT_END()

#define _PG_ASSERT2( X, MSG )                                                                \
    _PG_ASSERT_START( X )                                                                    \
    Logger_Log( LogSeverity::ERR, TerminalColorCode::RED, TerminalEmphasisCode::NONE, MSG ); \
    _PG_ASSERT_END()

#define _PG_ASSERT3( X, FMT, A1 )                                                                                   \
    _PG_ASSERT_START( X )                                                                                           \
    _PG_FMG_CHK1( FMT, A1 );                                                                                        \
    HANDLE_FORMAT_ERROR();                                                                                          \
    Logger_Log( LogSeverity::ERR, TerminalColorCode::RED, TerminalEmphasisCode::NONE, "Assert message: " FMT, A1 ); \
    _PG_ASSERT_END()

#define _PG_ASSERT4( X, FMT, A1, A2 )                                                                                   \
    _PG_ASSERT_START( X )                                                                                               \
    _PG_FMG_CHK2( FMT, A1, A2 );                                                                                        \
    HANDLE_FORMAT_ERROR();                                                                                              \
    Logger_Log( LogSeverity::ERR, TerminalColorCode::RED, TerminalEmphasisCode::NONE, "Assert message: " FMT, A1, A2 ); \
    _PG_ASSERT_END()

#define _PG_ASSERT5( X, FMT, A1, A2, A3 )                                                                                   \
    _PG_ASSERT_START( X )                                                                                                   \
    _PG_FMG_CHK3( FMT, A1, A2, A3 );                                                                                        \
    HANDLE_FORMAT_ERROR();                                                                                                  \
    Logger_Log( LogSeverity::ERR, TerminalColorCode::RED, TerminalEmphasisCode::NONE, "Assert message: " FMT, A1, A2, A3 ); \
    _PG_ASSERT_END()

#define _PG_ASSERT6( X, FMT, A1, A2, A3, A4 )                                                                                   \
    _PG_ASSERT_START( X )                                                                                                       \
    _PG_FMG_CHK4( FMT, A1, A2, A3, A4 );                                                                                        \
    HANDLE_FORMAT_ERROR();                                                                                                      \
    Logger_Log( LogSeverity::ERR, TerminalColorCode::RED, TerminalEmphasisCode::NONE, "Assert message: " FMT, A1, A2, A3, A4 ); \
    _PG_ASSERT_END()

#define _PG_ASSERT7( X, FMT, A1, A2, A3, A4, A5 )                                                                                   \
    _PG_ASSERT_START( X )                                                                                                           \
    _PG_FMG_CHK5( FMT, A1, A2, A3, A4, A5 );                                                                                        \
    HANDLE_FORMAT_ERROR();                                                                                                          \
    Logger_Log( LogSeverity::ERR, TerminalColorCode::RED, TerminalEmphasisCode::NONE, "Assert message: " FMT, A1, A2, A3, A4, A5 ); \
    _PG_ASSERT_END()

#define _PG_ASSERT8( X, FMT, A1, A2, A3, A4, A5, A6 )                                                                                   \
    _PG_ASSERT_START( X )                                                                                                               \
    _PG_FMG_CHK6( FMT, A1, A2, A3, A4, A5, A6 );                                                                                        \
    HANDLE_FORMAT_ERROR();                                                                                                              \
    Logger_Log( LogSeverity::ERR, TerminalColorCode::RED, TerminalEmphasisCode::NONE, "Assert message: " FMT, A1, A2, A3, A4, A5, A6 ); \
    _PG_ASSERT_END()

#define _PG_ASSERT9( X, FMT, A1, A2, A3, A4, A5, A6, A7 )                                                                           \
    _PG_ASSERT_START( X )                                                                                                           \
    _PG_FMG_CHK7( FMT, A1, A2, A3, A4, A5, A6, A7 );                                                                                \
    HANDLE_FORMAT_ERROR();                                                                                                          \
    Logger_Log(                                                                                                                     \
        LogSeverity::ERR, TerminalColorCode::RED, TerminalEmphasisCode::NONE, "Assert message: " FMT, A1, A2, A3, A4, A5, A6, A7 ); \
    _PG_ASSERT_END()

#define _PG_ASSERT10( X, FMT, A1, A2, A3, A4, A5, A6, A7, A8 )                                                                          \
    _PG_ASSERT_START( X )                                                                                                               \
    _PG_FMG_CHK8( FMT, A1, A2, A3, A4, A5, A6, A7, A8 );                                                                                \
    HANDLE_FORMAT_ERROR();                                                                                                              \
    Logger_Log(                                                                                                                         \
        LogSeverity::ERR, TerminalColorCode::RED, TerminalEmphasisCode::NONE, "Assert message: " FMT, A1, A2, A3, A4, A5, A6, A7, A8 ); \
    _PG_ASSERT_END()

#define _PG_ASSERT11( X, FMT, A1, A2, A3, A4, A5, A6, A7, A8, A9 )                                                                        \
    _PG_ASSERT_START( X )                                                                                                                 \
    _PG_FMG_CHK9( FMT, A1, A2, A3, A4, A5, A6, A7, A8, A9 );                                                                              \
    HANDLE_FORMAT_ERROR();                                                                                                                \
    Logger_Log( LogSeverity::ERR, TerminalColorCode::RED, TerminalEmphasisCode::NONE, "Assert message: " FMT, A1, A2, A3, A4, A5, A6, A7, \
        A8, A9 );                                                                                                                         \
    _PG_ASSERT_END()

#define _PG_ASSERT12( X, FMT, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10 )                                                                   \
    _PG_ASSERT_START( X )                                                                                                                 \
    _PG_FMG_CHK10( FMT, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10 );                                                                        \
    HANDLE_FORMAT_ERROR();                                                                                                                \
    Logger_Log( LogSeverity::ERR, TerminalColorCode::RED, TerminalEmphasisCode::NONE, "Assert message: " FMT, A1, A2, A3, A4, A5, A6, A7, \
        A8, A9, A10 );                                                                                                                    \
    _PG_ASSERT_END()

#else // #if USING( DEVELOPMENT_BUILD )

#define PG_DBG_ASSERT( ... ) \
    do                       \
    {                        \
    } while ( 0 )

#define PG_ASSERT( ... ) \
    do                   \
    {                    \
    } while ( 0 )

#endif // #else // #if USING( DEVELOPMENT_BUILD )
