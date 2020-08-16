#pragma once

#include "platform_defines.hpp"
#include <cstdio>
#include <string>

#if USING( SHIP_BUILD )

#define LOG( ... ) do {} while(0)
#define LOG_WARN( ... ) do {} while(0)
#define LOG_ERR( ... ) do {} while(0)

#else // #if USING( SHIP_BUILD )

#define LOG( ... ) Logger_Log( LogSeverity::DEBUG, __VA_ARGS__ )
#define LOG_WARN( ... ) Logger_Log( LogSeverity::WARN, __VA_ARGS__ )
#define LOG_ERR( ... ) Logger_Log( LogSeverity::ERR, __VA_ARGS__ )

#endif // #else // #if USING( SHIP_BUILD )

enum class LogSeverity
{
    DEBUG,
    WARN,
    ERR
};

void Logger_Init();

void Logger_Shutdown();

void Logger_AddLogLocation( const std::string& name, FILE* file, bool useColors = true );

void Logger_AddLogLocation( const std::string& name, const std::string& filename );
    
void Logger_RemoveLogLocation( const std::string& name );

void Logger_Log( LogSeverity sev, const char* fmt, ... );
