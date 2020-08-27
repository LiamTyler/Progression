#pragma once

#include <string>

// returns the last extension on a filename, including the period, lowercased.
// Ex: /foo/bar/baz.log.TXT -> .txt
std::string GetFileExtension( const std::string& filename );

// returns the base filename without the last extension or directories
// Ex: /foo/bar/baz.log.TXT -> baz.log
std::string GetFilenameStem( const std::string& filename );

// returns the filename and extension without any directories
// Ex: /foo/bar/baz.log.TXT -> baz.log.TXT
std::string GetRelativeFilename( const std::string& filename );

// returns everything before the filename and extension, with an ending forward slash
// Ex: /foo/bar/baz.log.TXT -> /foo/bar/
std::string GetParentPath( const std::string& filename );

bool FileExists( const std::string& filename );

// parent directory must exist.
// i.e: for /dir1/dir2, dir1 must be created first, then another call to create dir2
void CreateDirectory( const std::string& dir );

void DeleteFile( const std::string& filename );