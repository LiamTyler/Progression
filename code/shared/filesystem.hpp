#pragma once

#include <filesystem>
#include <string>
#include <vector>

struct FileReadResult
{
    FileReadResult() = default;
    ~FileReadResult() { Free(); }

    FileReadResult( const FileReadResult& obj )            = delete;
    FileReadResult& operator=( const FileReadResult& obj ) = delete;
    FileReadResult( FileReadResult&& obj );
    FileReadResult& operator=( FileReadResult&& obj );

    void Free();
    operator bool() const { return data != nullptr; }

    char* data  = nullptr;
    size_t size = 0;
};

FileReadResult ReadFile( std::string_view filename, bool binary = true );

bool WriteFile( std::string_view filename, char* data, size_t size, bool binary = true );

std::string BackToForwardSlashes( std::string str );

std::string UnderscorePath( std::string str );

// i.e: if dir='/dir1/dir2/dir3/' and only dir1 exists, then dir2 and dir3 will only be created
// if createParentDirs is true. Otherwise it will error
void CreateDirectory( const std::string& dir, bool createParentDirs = true );

// returns false if there was an error. If overwriteExisting is false, and 'to' exists, returns true
bool CopyFile( const std::string& from, const std::string& to, bool overwriteExisting );

// deletes single file or empty folder
void DeleteFile( const std::string& filename );

// Same as rm -rf
void DeleteRecursive( const std::string& path );

bool PathExists( const std::string& path );

bool IsDirectory( const std::string& path );

bool IsFile( const std::string& path );

std::string GetCWD();

std::string GetAbsolutePath( const std::string& path );

// Returns the last extension on a filename, including the period, lowercased. Same as std::filesystem::path::extension, then lowercased
// Ex: /foo/bar/baz.log.TXT -> .txt
std::string GetFileExtension( const std::string& filename );

// Ex: /foo/bar/tmp.txt -> /foo/bar/tmp
std::string GetFilenameMinusExtension( const std::string& filename );

// Returns the base filename without the last extension or directories. Same as std::filesystem::path::stem
// Ex: /foo/bar/baz.txt -> baz
// Ex: /foo/bar/baz.log.TXT -> baz.log
std::string GetFilenameStem( const std::string& filename );

// Returns the filename and extension without any directories. Same as std::filesystem::path::filename
// Ex: /foo/bar/baz.log.TXT -> baz.log.TXT
std::string GetRelativeFilename( const std::string& filename );

// Returns everything before the filename and extension, with an ending forward slash. Same as std::filesystem::path::parent_path
// except that it also works on directories
// Ex: /foo/bar/baz.log.TXT -> /foo/bar/
// Ex: /foo/bar/baz/ -> /foo/bar/
std::string GetParentPath( std::string path );

// Returns path relative to parentPath
// Ex: file = /foo/bar/baz/test.txt, relativeDir = /foo/bar/ -> baz/test.txt
std::string GetRelativePathToDir( const std::string& file, const std::string& relativeDir );

// Ex: path = /foo/bar/ -> bar
// Ex: path = /foo/bar/baz.txt -> bar
std::string GetDirectoryStem( const std::string& path );

std::vector<std::string> GetFilesInDir( const std::string& path, bool recursive );
