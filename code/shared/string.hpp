#pragma once

#include <string>
#include <vector>

int Stricmp( const char* str1, const char* str2 );
std::string StripWhitespace( const std::string& s );
// std::vector<std::string> SplitString( const std::string& str, const std::string delim = "," );
std::vector<std::string_view> SplitString( std::string_view str, std::string_view delim = "" );
void AddTrailingSlashIfMissing( std::string& str );
void SingleCharReplacement( char* str, char match, char replace );
