#include "utils/filesystem.hpp"
#include "utils/logger.hpp"
#include <filesystem>

#include <iostream>

namespace fs = std::filesystem;


std::string BackToForwardSlashes( std::string str )
{
    for ( size_t i = 0; i < str.length(); ++i )
    {
        if ( str[i] == '\\' )
        {
            str[i] = '/';
        }
    }

    return str;
}


void CreateDirectory( const std::string& dir )
{
    fs::create_directories( dir );
}


bool CopyFile( const std::string& from, const std::string& to, bool overwriteExisting )
{
    // For some reason, the overwrite_existing option doesnt work on my desktop
    //fs::copy_options options;
    //if ( overwriteExisting )
    //{
    //    options = fs::copy_options::overwrite_existing;
    //}
    //try
    //{
    //    fs::copy( from, to, fs::copy_options::overwrite_existing );
    //}
    //catch ( fs::filesystem_error &e )
    //{
    //    std::cout << "ERROR '" << e.what() << "'" << std::endl;
    //    std::cout << "Equivalent? '" << fs::equivalent( from, to ) << std::endl;
    //}
    if ( !overwriteExisting && FileExists( to ) )
    {
        return true;
    }

    FILE* infile = fopen( from.c_str(), "rb" );
    if ( infile == NULL )
    {
        return false;
    }
 
    fseek( infile, 0L, SEEK_END );
    long numBytes = ftell( infile );
    fseek( infile, 0L, SEEK_SET );
    char* buffer = (char*)malloc( numBytes );
    size_t numRead = fread( buffer, numBytes, 1, infile );
    if ( numRead != 1 )
    {
        return false;
    }
    fclose( infile );

    FILE* outFile = fopen( to.c_str(), "wb" );
    if ( outFile == NULL )
    {
        return false;
    }
    size_t numWritten = fwrite( buffer, numBytes, 1, outFile );
    if ( numWritten != 1 )
    {
        return false;
    }
    fclose( outFile );

    return true;
}


void DeleteFile( const std::string& filename )
{
    try
    {
        fs::remove( filename );
    }
    catch ( std::filesystem::filesystem_error& e )
    {
        PG_NO_WARN_UNUSED( e );
        LOG_ERR( "Failed to delete file. Error: '%s' and code '%d'\n", e.what(), e.code().value() );
    }
}


void DeleteRecursive( const std::string& path )
{
    fs::remove_all( path );
}


bool FileExists( const std::string& filename )
{
    return fs::is_regular_file( filename );
}


std::string GetCWD()
{
    return BackToForwardSlashes( fs::current_path().string() );
}


std::string GetAbsolutePath( const std::string& path )
{
    return BackToForwardSlashes( fs::absolute( path ).string() );
}


std::string GetFileExtension( const std::string& filename )
{
    std::string ext = fs::path( filename ).extension().string();
    for ( size_t i = 0; i < ext.length(); ++i )
    {
        ext[i] = std::tolower( ext[i] );
    }

    return ext;
}


std::string GetFilenameMinusExtension( const std::string& filename )
{
    return GetParentPath( filename ) + GetFilenameStem( filename );
}


std::string GetFilenameStem( const std::string& filename )
{
    return fs::path( filename ).stem().string();
}


std::string GetRelativeFilename( const std::string& filename )
{
    return fs::path( filename ).filename().string();
}


std::string GetParentPath( std::string path )
{
    if ( path.empty() )
    {
        return "";
    }
    
    path[path.length() - 1] = ' ';
    std::string parent = fs::path( path ).parent_path().string();
    if ( parent.length() )
    {
        parent += '/';
    }
    parent = BackToForwardSlashes( parent );

    return parent;
}


std::string GetRelativePathToDir( const std::string& file, const std::string& parentPath )
{
    return BackToForwardSlashes( fs::relative( file, parentPath ).string() );
}