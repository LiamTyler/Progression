#pragma once

#include <limits>
#include <string>
#include <vector>

#define NO_TIMESTAMP time_t{ 0 }
#define LATEST_TIMESTAMP std::numeric_limits<time_t>::max()

time_t GetFileTimestamp( const std::string& file );

// if file and dependency both exist, this returns true if the dependency's last modified time is newer than file's last modified time
// if file doesn't exist, will return true
// if file exists, and dependency doesnt, will return false
bool IsFileOutOfDate( const std::string& file, const std::string& dependency );
bool IsFileOutOfDate( const std::string& file, const std::string* dependencies, size_t numDependencies );
bool IsFileOutOfDate( const std::string& file, const std::vector< std::string >& dependencies );
bool IsFileOutOfDate( time_t timestamp, const std::string& dependentFile );
bool IsFileOutOfDate( time_t timestamp, const std::string* dependencies, size_t numDependencies );
bool IsFileOutOfDate( time_t timestamp, const std::vector< std::string >& dependencies );