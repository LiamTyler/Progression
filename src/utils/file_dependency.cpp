#include "file_dependency.hpp"
#include <sys/stat.h>


time_t GetFileTimestamp( const std::string& file )
{
    struct stat s;
    if ( stat( file.c_str(), &s ) == 0 )
    {
        return s.st_mtime;
    }

    return 0;
}


bool IsFileOutOfDate( const std::string& file, const std::string& dependency )
{
    time_t fTime = GetFileTimestamp( file );
    if ( fTime == 0 )
    {
        return true;
    }
    return fTime < GetFileTimestamp( dependency );
}


bool IsFileOutOfDate( const std::string& file, const std::string* dependencies, size_t numDependencies )
{
    time_t fileTime = GetFileTimestamp( file );
    for ( size_t i = 0; i < numDependencies; ++i )
    {
        if ( fileTime < GetFileTimestamp( dependencies[i] ) )
        {
            return true;
        }
    }

    return false;
}


bool IsFileOutOfDate( const std::string& file, const std::vector< std::string >& dependencies )
{
    return IsFileOutOfDate( file, dependencies.data(), dependencies.size() );
}