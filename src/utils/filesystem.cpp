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


std::string GetFilenameStem( const std::string& filename )
{
    return fs::path( filename ).stem().string();
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


bool FileExists( const std::string& filename )
{
    return fs::is_regular_file( filename );
}


void CreateDirectory( const std::string& dir )
{
    fs::create_directories( dir );
}


void DeleteFile( const std::string& filename )
{
    fs::remove( filename );
}