#pragma once

#include <string>


// returns the malloc'd buffer filled with the compressed data on success, nullptr on failure
// outputs compressSize
char* LZ4CompressBuffer( const char* uncompressedData, size_t size, int& compressedSize );

char* LZ4DecompressBuffer( const char* compressedData, int compressedSize, int uncompressedSize );

bool LZ4CompressFile( const std::string& inputFilename, const std::string& outputFilename );