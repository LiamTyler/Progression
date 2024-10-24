#include "string.hpp"
#include "shared/platform_defines.hpp"
#if USING( LINUX_PROGRAM )
#include <strings.h>
#endif // #if USING( LINUX_PROGRAM )

i32 Stricmp( const char* str1, const char* str2 )
{
#if USING( WINDOWS_PROGRAM )
    return _stricmp( str1, str2 );
#else  // #if USING( WINDOWS_PROGRAM )
    return strcasecmp( str1, str2 );
#endif // #else // #if USING( WINDOWS_PROGRAM )
}

i32 Strncmp( const char* str1, const char* str2, i32 n ) { return strncmp( str1, str2, n ); }

std::string StripWhitespace( const std::string& s )
{
    size_t start, end;
    for ( start = 0; start < s.length() && std::isspace( s[start] ); ++start )
        ;
    for ( end = s.length() - 1; end >= 0 && std::isspace( s[end] ); --end )
        ;

    return s.substr( start, end - start + 1 );
}

std::vector<std::string_view> SplitString( std::string_view str, std::string_view delim )
{
    if ( str.empty() )
        return {};

    std::vector<std::string_view> ret;
    size_t start_index = 0;
    size_t index       = 0;
    while ( ( index = str.find_first_of( delim, start_index ) ) != std::string_view::npos )
    {
        ret.push_back( str.substr( start_index, index - start_index ) );
        start_index = index + 1;

        if ( index == str.size() - 1 )
        {
            ret.emplace_back();
        }
    }

    if ( start_index < str.size() )
    {
        ret.push_back( str.substr( start_index ) );
    }
    return ret;
}

void AddTrailingSlashIfMissing( std::string& str )
{
    if ( str[str.length() - 1] != '/' && str[str.length() - 1] != '\\' )
    {
        str += '/';
    }
}

void SingleCharReplacement( char* str, char match, char replace )
{
    while ( *str != '\0' )
    {
        if ( *str == match )
            *str = replace;

        ++str;
    }
}
