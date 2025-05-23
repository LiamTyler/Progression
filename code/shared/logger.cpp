#include "logger.hpp"
#include "shared/assert.hpp"
#include "shared/platform_defines.hpp"
#include <cstring>
#include <mutex>
#include <stdarg.h>
#include <stdio.h>
#if USING( WINDOWS_PROGRAM )
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <io.h>
#endif // #if USING( WINDOWS_PROGRAM )

// For special windows consoles like cmd.exe that use the SetConsoleTextAttribute function for coloring
i32 TerminalCodesToWindowsCodes( TerminalColorCode color, TerminalEmphasisCode emphasis )
{
    static i32 colorToWindows[] = {
        0, // BLACK
        4, // RED
        2, // GREEN
        6, // YELLOW
        1, // BLUE
        5, // MAGENTA
        3, // CYAN
        7, // WHITE
    };

    i32 windowsCode = colorToWindows[static_cast<i32>( color ) - 30];
    if ( emphasis == TerminalEmphasisCode::BOLD )
    {
        windowsCode += 8;
    }

    return windowsCode;
}

class LoggerOutputLocation
{
public:
    LoggerOutputLocation() = default;

    LoggerOutputLocation( std::string_view name, std::FILE* out, bool useColors = true )
        : m_name( name ), m_outputStream( out ), m_colored( useColors )
    {
        m_windowsConsole = false;
#if USING( WINDOWS_PROGRAM )
        if ( _isatty( _fileno( out ) ) )
        {
            PG_ASSERT( out == stdout || out == stderr, "hConsole in the logging function relies on this" );
            m_windowsConsole = true;
        }
#endif // #if USING( WINDOWS_PROGRAM )
    }

    LoggerOutputLocation( std::string_view name, std::string_view filename )
        : m_name( name ), m_outputStream( fopen( filename.data(), "w" ) ), m_colored( false ), m_windowsConsole( false )
    {
        if ( m_outputStream == nullptr )
        {
            printf( "Could not open file '%s'\n", filename.data() );
        }
    }

    LoggerOutputLocation( const LoggerOutputLocation& )            = delete;
    LoggerOutputLocation& operator=( const LoggerOutputLocation& ) = delete;

    LoggerOutputLocation( LoggerOutputLocation&& log ) { *this = std::move( log ); }

    LoggerOutputLocation& operator=( LoggerOutputLocation&& log )
    {
        static_assert( sizeof( LoggerOutputLocation ) == sizeof( std::string ) + 16,
            "Don't forget to update this if adding new params to LoggerOutputLocation" );
        Close();
        m_name             = std::move( log.m_name );
        m_outputStream     = std::move( log.m_outputStream );
        log.m_outputStream = NULL;
        m_colored          = std::move( log.m_colored );
        m_windowsConsole   = std::move( log.m_windowsConsole );

        return *this;
    }

    ~LoggerOutputLocation() { Close(); }

    void Close()
    {
        if ( m_outputStream != NULL && m_outputStream != stdout && m_outputStream != stderr )
        {
            fclose( m_outputStream );
            m_outputStream = NULL;
        }
    }

    std::string_view GetName() const { return m_name; }
    std::FILE* GetOutputFile() const { return m_outputStream; }
    bool IsOutputColored() const { return m_colored; }
    void SetOutputColored( bool color ) { m_colored = color; };
    bool IsSpecialWindowsConsole() const { return m_windowsConsole; }

private:
    std::string m_name;
    std::FILE* m_outputStream = nullptr;
    bool m_colored            = true;
    bool m_windowsConsole     = false;
};

#define MAX_NUM_LOGGER_OUTPUT_LOCATIONS 10
static std::mutex s_loggerLock;
static LoggerOutputLocation s_loggerLocations[MAX_NUM_LOGGER_OUTPUT_LOCATIONS];
static i32 s_numLogs = 0;

void Logger_Init() {}

void Logger_Shutdown()
{
    for ( i32 i = 0; i < s_numLogs; ++i )
    {
        s_loggerLocations[i].Close();
    }
    s_numLogs = 0;
}

void Logger_AddLogLocation( std::string_view name, FILE* file, bool useColors )
{
    PG_ASSERT( s_numLogs != MAX_NUM_LOGGER_OUTPUT_LOCATIONS );
    s_loggerLock.lock();
    s_loggerLocations[s_numLogs] = LoggerOutputLocation( name, file, useColors );
    ++s_numLogs;
    s_loggerLock.unlock();
}

void Logger_AddLogLocation( std::string_view name, std::string_view filename )
{
    PG_ASSERT( s_numLogs != MAX_NUM_LOGGER_OUTPUT_LOCATIONS );
    s_loggerLock.lock();
    s_loggerLocations[s_numLogs] = LoggerOutputLocation( name, filename );
    ++s_numLogs;
    s_loggerLock.unlock();
}

void Logger_RemoveLogLocation( std::string_view name )
{
    s_loggerLock.lock();
    for ( i32 i = 0; i < s_numLogs; ++i )
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

void Logger_ChangeLocationColored( std::string_view name, bool colored )
{
    s_loggerLock.lock();
    for ( i32 i = 0; i < s_numLogs; ++i )
    {
        if ( name == s_loggerLocations[i].GetName() )
        {
            s_loggerLocations[i].SetOutputColored( colored );
            break;
        }
    }
    s_loggerLock.unlock();
}

void Logger_Log( LogSeverity severity, TerminalColorCode color, TerminalEmphasisCode emphasis, const char* fmt, ... )
{
    std::string_view severityText = "";
    if ( severity == LogSeverity::WARN )
    {
        severityText = "WARNING  ";
    }
    else if ( severity == LogSeverity::ERR )
    {
        severityText = "ERROR    ";
    }

    char colorEncoding[12];
    sprintf( colorEncoding, "\033[%d;%dm", static_cast<i32>( emphasis ), static_cast<i32>( color ) );
    char fullFormat[512];
    size_t formatLen  = strlen( fmt );
    char* currentSpot = fullFormat;
    memcpy( currentSpot, severityText.data(), severityText.length() );
    currentSpot += severityText.length();
    memcpy( currentSpot, fmt, formatLen );
    currentSpot += formatLen;
    *currentSpot = '\n';
    currentSpot += 1;
    *currentSpot = '\0';

    s_loggerLock.lock();
    for ( i32 i = 0; i < s_numLogs; ++i )
    {
        va_list args;
        va_start( args, fmt );
        if ( s_loggerLocations[i].IsOutputColored() )
        {
#if USING( WINDOWS_PROGRAM )
            if ( s_loggerLocations[i].IsSpecialWindowsConsole() )
            {
                i32 windowsCode = TerminalCodesToWindowsCodes( color, emphasis );
                HANDLE hConsole = GetStdHandle( s_loggerLocations[i].GetOutputFile() == stdout ? STD_OUTPUT_HANDLE : STD_ERROR_HANDLE );
                SetConsoleTextAttribute( hConsole, windowsCode );
                vfprintf( s_loggerLocations[i].GetOutputFile(), fullFormat, args );
                SetConsoleTextAttribute( hConsole, 7 ); // non-emphasized white
            }
            else
            {
                fprintf( s_loggerLocations[i].GetOutputFile(), colorEncoding );
                vfprintf( s_loggerLocations[i].GetOutputFile(), fullFormat, args );
                fprintf( s_loggerLocations[i].GetOutputFile(), "\033[0m" );
            }
#else  // #if USING( WINDOWS_PROGRAM )
            fprintf( s_loggerLocations[i].GetOutputFile(), colorEncoding );
            vfprintf( s_loggerLocations[i].GetOutputFile(), fullFormat, args );
            fprintf( s_loggerLocations[i].GetOutputFile(), "\033[0m" );
#endif // #else // #if USING( WINDOWS_PROGRAM )
        }
        else
        {
            vfprintf( s_loggerLocations[i].GetOutputFile(), fullFormat, args );
        }
        // newlines don't necessarily flush the output. Ex: git bash
        fflush( s_loggerLocations[i].GetOutputFile() );
        va_end( args );
    }
    s_loggerLock.unlock();
}
