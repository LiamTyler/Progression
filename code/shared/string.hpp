#pragma once

#include <string>
#include <vector>

std::string StripWhitespace( const std::string& s );
std::vector<std::string> SplitString( const std::string& str, const std::string delim = "," );