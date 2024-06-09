#pragma once

#include "shared/core_defines.hpp"
#include <cstring>
#include <string>
#include <vector>

i32 Stricmp( const char* str1, const char* str2 );
i32 Strncmp( const char* str1, const char* str2, i32 n );
std::string StripWhitespace( const std::string& s );
std::vector<std::string_view> SplitString( std::string_view str, std::string_view delim = "," );
void AddTrailingSlashIfMissing( std::string& str );
void SingleCharReplacement( char* str, char match, char replace );
