#pragma once

#include <string>
#include <vector>

time_t GetFileTimestamp( const std::string& file );

// if file and dependency both exist, this returns true if the dependency's last modified time is newer than file's last modified time
// if file doesn't exist, will return true
// if file exists, and dependency doesnt, will return false
bool IsFileOutOfDate( const std::string& file, const std::string& dependency );

bool IsFileOutOfDate( const std::string& file, const std::vector< std::string >& dependencies );