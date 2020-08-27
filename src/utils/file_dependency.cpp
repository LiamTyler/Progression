#include "file_dependency.hpp"
#include <sys/stat.h>

static time_t GetFileTime( const std::string& file )
{
    struct stat s;
    if ( stat( file.c_str(), &s ) == 0 )
    {
        return s.st_mtime;
    }

    return 0;
}

namespace Progression
{


bool IsFileOutOfDate( const std::string& file, const std::string& dependency )
{
    return GetFileTime( file ) < GetFileTime( dependency );
}


bool IsFileOutOfDate( const std::string& file, const std::vector< std::string >& dependencies )
{
    time_t fileTime = GetFileTime( file );
    for ( const std::string& dependency : dependencies )
    {
        if ( fileTime < GetFileTime( dependency ) )
        {
            return true;
        }
    }

    return false;
}


} // namespace Progression