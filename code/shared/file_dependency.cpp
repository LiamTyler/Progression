#include "file_dependency.hpp"
#include <sys/stat.h>

time_t GetFileTimestamp( const std::string& file )
{
    if ( file.empty() )
    {
        return NO_TIMESTAMP;
    }

    struct stat s;
    if ( stat( file.c_str(), &s ) == 0 )
    {
        return s.st_mtime;
    }

    return NO_TIMESTAMP;
}

bool IsFileOutOfDate( const std::string& file, const std::string& dependentFile )
{
    return IsFileOutOfDate( GetFileTimestamp( file ), dependentFile );
}

bool IsFileOutOfDate( const std::string& file, const std::string* dependencies, size_t numDependencies )
{
    time_t fileTime = GetFileTimestamp( file );
    return IsFileOutOfDate( fileTime, dependencies, numDependencies );
}

bool IsFileOutOfDate( const std::string& file, const std::vector<std::string>& dependencies )
{
    return IsFileOutOfDate( file, dependencies.data(), dependencies.size() );
}

bool IsFileOutOfDate( time_t timestamp, const std::string& dependentFile )
{
    if ( timestamp == NO_TIMESTAMP )
        return true;
    auto dependentTimestamp = GetFileTimestamp( dependentFile );
    return dependentTimestamp == NO_TIMESTAMP || timestamp < dependentTimestamp;
}

bool IsFileOutOfDate( time_t timestamp, const std::string* dependencies, size_t numDependencies )
{
    if ( timestamp == NO_TIMESTAMP )
        return true;

    for ( size_t i = 0; i < numDependencies; ++i )
    {
        auto dependentTimestamp = GetFileTimestamp( dependencies[i] );
        if ( dependentTimestamp == NO_TIMESTAMP || timestamp < dependentTimestamp )
        {
            return true;
        }
    }

    return false;
}

bool IsFileOutOfDate( time_t timestamp, const std::vector<std::string>& dependencies )
{
    return IsFileOutOfDate( timestamp, dependencies.data(), dependencies.size() );
}
