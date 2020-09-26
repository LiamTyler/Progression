#include "utils/logger.hpp"
#include "core/assert.hpp"
#include <cstring>
#include <mutex>
#include <stdarg.h>
#include <stdio.h>

enum class TerminalColorCode
{
    RED     = 31,
    GREEN   = 32,
    YELLOW  = 33,
    BLUE    = 34,
    DEFAULT = 39
};

enum class TerminalEmphasisCode
{
    NONE      = 0,
    BOLD      = 1,
    UNDERLINE = 4
};

class LoggerOutputLocation
{
public:
    LoggerOutputLocation() = default;

    LoggerOutputLocation( const std::string& name, std::FILE* out, bool useColors = true ) : m_name( name ), m_outputStream( out ), m_colored( useColors ) {}

    LoggerOutputLocation( const std::string& name, const std::string& filename ) :
        m_name( name ), m_outputStream( fopen( filename.c_str(), "w" ) ), m_colored( false )
    {
        if ( m_outputStream == nullptr )
        {
            printf( "Could not open file '%s'\n", filename.c_str() );
        }
    }

    LoggerOutputLocation( const LoggerOutputLocation& ) = delete;
    LoggerOutputLocation& operator=( const LoggerOutputLocation& ) = delete;

    LoggerOutputLocation( LoggerOutputLocation&& log )
    {
        *this = std::move( log );
    }

    LoggerOutputLocation& operator=( LoggerOutputLocation&& log )
    {
        Close();
        m_name         = std::move( log.m_name );
        m_outputStream = std::move( log.m_outputStream );
        log.m_outputStream = NULL;
        m_colored      = std::move( log.m_colored );

        return *this;
    }

    ~LoggerOutputLocation()
    {
        Close();
    }

    void Close()
    {
        if ( m_outputStream != NULL && m_outputStream != stdout )
        {
            fclose( m_outputStream );
            m_outputStream = NULL;
        }
    }

    std::string GetName() const { return m_name; }
    std::FILE* GetOutputFile() const { return m_outputStream; }
    bool IsOutputColored() const { return m_colored; }
    void SetOutputColored( bool color ) { m_colored = color; };

private:
    std::string m_name;
    std::FILE* m_outputStream = nullptr;
    bool m_colored = true;
};


#define MAX_NUM_LOGGER_OUTPUT_LOCATIONS 10
static std::mutex s_loggerLock;
static LoggerOutputLocation s_loggerLocations[MAX_NUM_LOGGER_OUTPUT_LOCATIONS];
static int s_numLogs = 0;


void Logger_Init()
{
}


void Logger_Shutdown()
{
    for ( int i = 0; i < s_numLogs; ++i )
    {
        s_loggerLocations[i].Close();
    }
    s_numLogs = 0;
}


void Logger_AddLogLocation( const std::string& name, FILE* file, bool useColors )
{
    PG_ASSERT( s_numLogs != MAX_NUM_LOGGER_OUTPUT_LOCATIONS );
    s_loggerLock.lock();
    s_loggerLocations[s_numLogs] = LoggerOutputLocation( name, file, useColors );
    ++s_numLogs;
    s_loggerLock.unlock();
}


void Logger_AddLogLocation( const std::string& name, const std::string& filename )
{
    PG_ASSERT( s_numLogs != MAX_NUM_LOGGER_OUTPUT_LOCATIONS );
    s_loggerLock.lock();
    s_loggerLocations[s_numLogs] = LoggerOutputLocation( name, filename );
    ++s_numLogs;
    s_loggerLock.unlock();
}


void Logger_RemoveLogLocation( const std::string& name )
{
    s_loggerLock.lock();
    for ( int i = 0; i < s_numLogs; ++i )
    {
        if ( name == s_loggerLocations[i].GetName() )
        {
            s_loggerLocations[i].Close();
            --s_numLogs;
            if ( i != s_numLogs )
            {
                s_loggerLocations[i] = std::move( s_loggerLocations[s_numLogs] );
            }

            break;
        }
    }
    s_loggerLock.unlock();
}


void Logger_ChangeLocationColored( const std::string& name, bool colored )
{
    s_loggerLock.lock();
    for ( int i = 0; i < s_numLogs; ++i )
    {
        if ( name == s_loggerLocations[i].GetName() )
        {
            s_loggerLocations[i].SetOutputColored( colored );
            break;
        }
    }
    s_loggerLock.unlock();
}


void Logger_Log( LogSeverity severity, const char* fmt, ... )
{
    std::string severityText = "";
    TerminalColorCode colorCode = TerminalColorCode::GREEN;
    TerminalEmphasisCode emphasisCode = TerminalEmphasisCode::NONE;
    if ( severity == LogSeverity::WARN )
    {
        severityText = "WARNING  ";
        colorCode = TerminalColorCode::YELLOW;
    }
    else if ( severity == LogSeverity::ERR )
    {
        severityText = "ERROR    ";
        colorCode = TerminalColorCode::RED;
    }

    char colorEncoding[12];
    sprintf( colorEncoding, "\033[%d;%dm", static_cast< int >( emphasisCode ), static_cast< int >( colorCode ) );
    char fullFormat[256];
    memcpy( fullFormat, severityText.c_str(), severityText.length() );
    strcpy( fullFormat + severityText.length(), fmt );

    s_loggerLock.lock();
    for ( int i = 0; i < s_numLogs; ++i )
    {
        va_list args;
        va_start( args, fmt );
        if ( s_loggerLocations[i].IsOutputColored() )
        {
            fprintf( s_loggerLocations[i].GetOutputFile(), colorEncoding );
            vfprintf( s_loggerLocations[i].GetOutputFile(), fullFormat, args );
            fprintf( s_loggerLocations[i].GetOutputFile(), "\033[0m" );
        }
        else
        {
            vfprintf( s_loggerLocations[i].GetOutputFile(), fullFormat, args );
        }
        va_end( args );
    }
    s_loggerLock.unlock();
}
