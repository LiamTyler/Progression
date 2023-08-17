#include "shared/lz4_compressor.hpp"
#include "shared/assert.hpp"
#include "shared/logger.hpp"
#include "lz4/lz4.h"
#include "memory_map/MemoryMapped.h"
#include <fstream>


char* LZ4CompressBuffer( const char* uncompressedData, size_t size, int& compressedSize )
{
    return nullptr;
}

char* LZ4DecompressBuffer( const char* compressedData, int compressedSize, int uncompressedSize )
{
    // char* uncompressedBuffer   = (char*) malloc( uncompressedSize );
    // const int decompressedSize = LZ4_decompress_safe( compressedData, uncompressedBuffer, compressedSize, uncompressedSize );
    return nullptr;
}

bool LZ4CompressFile( const std::string& inputFilename, const std::string& outputFilename )
{
    MemoryMapped memMappedFile;
    if ( !memMappedFile.open( inputFilename, MemoryMapped::WholeFile, MemoryMapped::Normal ) )
    {
        LOG_ERR( "Could not open file: '%s'", inputFilename.c_str() );
        return false;
    }

    char* src = (char*) memMappedFile.getData();
    const int srcSize = (int) memMappedFile.size();
    const int maxDstSize = LZ4_compressBound( srcSize );

    char* compressedData = (char*) malloc( maxDstSize );
    const int compressedDataSize = LZ4_compress_default( src, compressedData, srcSize, maxDstSize );

    memMappedFile.close();

    if ( compressedDataSize <= 0 )
    {
        LOG_ERR( "Error while trying to compress the file. LZ4 returned: %d", compressedDataSize );
        return false;
    }

    if ( compressedDataSize > 0 )
    {
        LOG( "Compressed file size ratio: %.3f", (float) compressedDataSize / srcSize );
    }

    std::ofstream out( outputFilename, std::ios::binary );
    if ( !out )
    {
        LOG_ERR( "Failed to open file '%s' for writing compressed results", outputFilename.c_str() );
        return false;
    }

    out.write( (char*)&srcSize, sizeof( srcSize ) );
    out.write( compressedData, compressedDataSize );

    return true;
}