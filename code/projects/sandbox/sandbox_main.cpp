#include "shared/bitops.hpp"
#include "shared/filesystem.hpp"
#include "shared/logger.hpp"
#include "shared/lz4_compressor.hpp"
#include <iostream>

using namespace std;

// currently compression for assets:
// images: 0-2% default, ~7% HC (no RDO currently)
// models: varies a lot. Some are 18-24%, some are 37-45%. todo: investigate meshopt_encode[Index/Vertex]Buffer
// scripts: ~50%
// shaders: ~50-60%
// pipelines: ~2%
// materials: 17%

int main( int argc, char* argv[] )
{
    Logger_Init();
    Logger_AddLogLocation( "stdout", stdout );
    u32 a = 0b00010;
    LOG( "TZ: %u", ctz( a ) );

    // FileReadResult readRes =
    //     ReadFile( "C:\\Users\\Liam\\Documents\\Progression\\assets\\models\\sponza_intel\\NewSponza_Main_glTF_002.pmodelb" );
    // if ( !readRes )
    //{
    //     LOG_ERR( "Couldnt read file" );
    //     return 0;
    // }
    //
    // float toMB = 1.0f / ( 1024 * 1024 );
    //
    // int default_compressedSize   = 0;
    // char* default_compressedData = LZ4CompressBuffer( readRes.data, readRes.size, default_compressedSize );
    // float diff                   = 1.0f - ( default_compressedSize / (float)readRes.size );
    // LOG( "Size before %.3fMB, after %.3fMB, savings: %.2f%%", readRes.size * toMB, default_compressedSize * toMB, 100 * diff );
    //
    // int hc_compressedSize   = 0;
    // char* hc_compressedData = LZ4CompressBufferHC( readRes.data, readRes.size, 12, hc_compressedSize );
    // diff                    = 1.0f - ( hc_compressedSize / (float)readRes.size );
    // LOG( "HC Size before %.3fMB, after %.3fMB, savings: %.2f%%", readRes.size * toMB, hc_compressedSize * toMB, 100 * diff );

    return 0;
}
