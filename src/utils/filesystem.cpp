#include "utils/filesystem.hpp"
#include <filesystem>

namespace fs = std::filesystem;


std::string GetFileExtension( const std::string& filename )
{
    std::string ext = fs::path( filename ).extension().string();
    for ( size_t i = 0; i < ext.length(); ++i )
    {
        ext[i] = std::tolower( ext[i] );
    }
    return ext;
}


std::string GetRelativeFilename( const std::string& filename )
{
    return fs::path( filename ).filename().string();
}


std::string GetParentPath( const std::string& filename )
{
    std::string path = fs::path( filename ).parent_path().string();
    if ( path .length() )
    {
        path  += '/';
    }
    return path ;
}