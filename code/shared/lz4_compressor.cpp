#include "shared/lz4_compressor.hpp"
#include "lz4/lz4.h"
#include "lz4/lz4hc.h"
#include "memory_map/MemoryMapped.h"
#include "shared/assert.hpp"
#include "shared/logger.hpp"
#include <fstream>

char* LZ4CompressBuffer( const char* uncompressedData, size_t size, i32& compressedSize )
{
    if ( size > LZ4_MAX_INPUT_SIZE )
    {
        LOG( "Buffer too large to compress. Max input size is %d", LZ4_MAX_INPUT_SIZE );
        compressedSize = 0;
        return nullptr;
    }
    const i32 maxDstSize = LZ4_compressBound( (int)size );
    char* compressedData = (char*)malloc( maxDstSize );
    compressedSize = LZ4_compress_default( uncompressedData, compressedData, (int)size, maxDstSize );

    if ( compressedSize <= 0 )
    {
        LOG_ERR( "Error while trying to compress the buffer. LZ4 returned: %d", compressedSize );
        free( compressedData );
        return nullptr;
    }

    return compressedData;
}

char* LZ4CompressBufferHC( const char* uncompressedData, size_t size, int compressionLevel, int& compressedSize )
{
    if ( size > LZ4_MAX_INPUT_SIZE )
    {
        LOG( "Buffer too large to compress. Max input size is %d", LZ4_MAX_INPUT_SIZE );
        compressedSize = 0;
        return nullptr;
    }
    const i32 maxDstSize = LZ4_compressBound( (int)size );
    char* compressedData = (char*)malloc( maxDstSize );
    compressedSize = LZ4_compress_HC( uncompressedData, compressedData, (int)size, maxDstSize, compressionLevel );

    if ( compressedSize <= 0 )
    {
        LOG_ERR( "Error while trying to compress the buffer. LZ4 returned: %d", compressedSize );
        free( compressedData );
        return nullptr;
    }

    return compressedData;
}

char* LZ4DecompressBuffer( const char* compressedData, i32 compressedSize, i32 uncompressedSize )
{
    char* uncompressedBuffer   = (char*) malloc( uncompressedSize );
    const i32 decompressedSize = LZ4_decompress_safe( compressedData, uncompressedBuffer, compressedSize, uncompressedSize );
    if ( decompressedSize != compressedSize )
    {
        LOG_ERR( "Error while decompressing. LZ4 returned: %d", decompressedSize );
        free( uncompressedBuffer );
        return nullptr;
    }

    return uncompressedBuffer;
}

bool LZ4CompressFile( const std::string& inputFilename, const std::string& outputFilename )
{
    MemoryMapped memMappedFile;
    if ( !memMappedFile.open( inputFilename, MemoryMapped::WholeFile, MemoryMapped::Normal ) )
    {
        LOG_ERR( "Could not open file: '%s'", inputFilename.c_str() );
        return false;
    }

    char* src            = (char*)memMappedFile.getData();
    const i32 srcSize    = (i32)memMappedFile.size();
    const i32 maxDstSize = LZ4_compressBound( srcSize );

    char* compressedData         = (char*)malloc( maxDstSize );
    const i32 compressedDataSize = LZ4_compress_default( src, compressedData, srcSize, maxDstSize );

    memMappedFile.close();

    if ( compressedDataSize <= 0 )
    {
        LOG_ERR( "Error while trying to compress the file. LZ4 returned: %d", compressedDataSize );
        return false;
    }

    if ( compressedDataSize > 0 )
    {
        LOG( "Compressed file size ratio: %.3f", (f32)compressedDataSize / srcSize );
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
