#pragma once

#include <string>
#include <vector>

int Stricmp( const char* str1, const char* str2 );
std::string StripWhitespace( const std::string& s );
std::vector<std::string> SplitString( const std::string& str, const std::string delim = "," );