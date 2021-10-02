#include "bc_compression.hpp"
#include "bc7enc_rdo/rgbcx.h"
#include "bc7enc_rdo/bc7e_ispc.h"
#include "compressonator/cmp_core/source/cmp_core.h"
#include "image.hpp"
#include "shared/assert.hpp"
#include "shared/logger.hpp"


int NumBytesPerCompressedBlock( CompressionFormat format )
{
    PG_ASSERT( static_cast<int>( format ) <= static_cast<int>( CompressionFormat::BC7_SRGB ) && format != CompressionFormat::INVALID );
    int size[] =
    {
        8,  // BC1_UNORM
        8,  // BC1_SRGB
        16, // BC2_UNORM
        16, // BC2_SRGB
        16, // BC3_UNORM
        16, // BC3_SRGB
        8,  // BC4_UNORM
        8,  // BC4_SNORM
        16, // BC5_UNORM
        16, // BC5_SNORM
        16, // BC6H_UFLOAT
        16, // BC6H_SFLOAT
        16, // BC7_UNORM
        16, // BC7_SRGB
    };
    return size[static_cast<int>( format ) - static_cast<int>( CompressionFormat::BC1_UNORM )];
}


int GetBCNumber( CompressionFormat format )
{
    return 1 + static_cast<int>( format ) / 2;
}


static bool IsValidFormat( CompressionFormat format )
{
    if ( static_cast<int>( format ) > static_cast<int>( CompressionFormat::BC7_SRGB ) || format == CompressionFormat::INVALID )
    {
        LOG_ERR( "Please specify valid output BC format" );
    }
    if ( format == CompressionFormat::BC2_SRGB || format == CompressionFormat::BC2_UNORM )
    {
        LOG_ERR( "BC2 compression not supported. BC3 has same memory use and is higher quality, use that instead." );
        return false;
    }
    if ( format == CompressionFormat::BC6H_SFLOAT || format == CompressionFormat::BC6H_UFLOAT )
    {
        LOG_ERR( "BC6 not implemented yet!" );
        return false;
    }

    return true;
}


bool Compress_RGBASingleMip_To_BC( uint8_t* srcMip, int width, int height, const BCCompressorSettings& settings, uint8_t* compressedOutput )
{
    if ( !srcMip || !IsValidFormat( settings.format ) )
    {
        return false;
    }

    const int blocksY = (height + 3) / 4;
    const int blocksX = (width + 3) / 4;
    const int totalBlocks = blocksX * blocksY;
    const int bytesPerBlock = NumBytesPerCompressedBlock( settings.format );
    const int bcNumber = GetBCNumber( settings.format );

    if ( bcNumber <= 5 )
    {
        rgbcx::init();
        ImageU8 image( width, height, srcMip );

        uint32_t bc1Level = rgbcx::MIN_LEVEL;
        uint32_t bc4Level = 1;
        if ( settings.quality == CompressionQuality::MEDIUM )
        {
            bc1Level = 10;
            bc4Level = 4;
        }
        else if ( settings.quality == CompressionQuality::HIGHEST )
        {
            bc1Level = rgbcx::MAX_LEVEL;
            bc4Level = 12;
        }
        bool isLowQuality = settings.quality == CompressionQuality::LOWEST;
    
        #pragma omp parallel for
	    for ( int by = 0; by < blocksY; ++by )
	    {
		    for ( int bx = 0; bx < blocksX; ++bx )
		    {
                glm::u8vec4 src[16];
                image.GetBlockClamped( bx, by, &src[0] );
                uint8_t* dst = compressedOutput + bytesPerBlock * (by * blocksX + bx);
                switch ( bcNumber )
                {
                case 1:
                    rgbcx::encode_bc1( bc1Level, dst, &src[0][0], true, true );
                    break;
                case 3:
                    if ( isLowQuality ) rgbcx::encode_bc3( bc1Level, dst, &src[0][0] );
                    else rgbcx::encode_bc3_hq( bc1Level, dst, &src[0][0], bc4Level );
                    break;
                case 4:
                    if ( isLowQuality ) rgbcx::encode_bc4( dst, &src[0][settings.bc4SourceChannel] );
                    else rgbcx::encode_bc4_hq( dst, &src[0][settings.bc4SourceChannel], 4, bc4Level );
                    break;
                case 5:
                    if ( isLowQuality ) rgbcx::encode_bc5( dst, &src[0][0], settings.bc5SourceChannel1, settings.bc5SourceChannel2 );
                    else rgbcx::encode_bc5_hq( dst, &src[0][0], settings.bc5SourceChannel1, settings.bc5SourceChannel2, 4, bc4Level );
                    break;
                }
            }
        }
    }
    else if ( bcNumber == 6 )
    {
        ImageF16 image( width, height, srcMip );
        void* options = nullptr;
        CreateOptionsBC6( &options );
        SetQualityBC6( options, (float)settings.quality / (float)CompressionQuality::HIGHEST); // quality is continuous in [0,1], the higher the better

        #pragma omp parallel for
	    for ( int by = 0; by < blocksY; ++by )
	    {
		    for ( int bx = 0; bx < blocksX; ++bx )
		    {
                glm::u16vec4 blockSrc[16];
                image.GetBlockClamped( bx, by, &blockSrc[0] );
                uint8_t* blockDst = compressedOutput + bytesPerBlock * (by * blocksX + bx);
                CompressBlockBC6( (uint16_t*)blockSrc, 16, blockDst, options );
            }
        }
        DestroyOptionsBC6( options );
    }
    else
    {
        ImageU8 image( width, height, srcMip );
        ispc::bc7e_compress_block_init();
        ispc::bc7e_compress_block_params params;
        if ( settings.quality == CompressionQuality::LOWEST ) ispc::bc7e_compress_block_params_init_ultrafast( &params, true );
        else if ( settings.quality == CompressionQuality::MEDIUM ) ispc::bc7e_compress_block_params_init_basic( &params, true );
        else ispc::bc7e_compress_block_params_init_slowest( &params, true );
        
        // bc7e wants an array of 4x4 blocks, at least 32-64 at a time if possible
        std::vector<glm::u8vec4[16]> blocks( totalBlocks );
        #pragma omp parallel for
        for ( int by = 0; by < blocksY; ++by )
	    {
		    for ( int bx = 0; bx < blocksX; ++bx )
		    {
                image.GetBlockClamped( bx, by, &blocks[by*blocksX + bx][0] );
            }
        }
        
        const int blocksPerChunk = 64;
        const int chunks = (totalBlocks + blocksPerChunk - 1) / blocksPerChunk;
        #pragma omp parallel for
        for ( int chunk = 0; chunk < chunks; ++chunk )
	    {
            uint64_t* dstBlock = reinterpret_cast<uint64_t*>( compressedOutput + chunk * blocksPerChunk * bytesPerBlock );
            uint32_t numBlocks = std::min( blocksPerChunk, totalBlocks - chunk * blocksPerChunk );
            uint32_t* srcBlock = reinterpret_cast<uint32_t*>( &blocks[chunk * blocksPerChunk][0][0] );
            ispc::bc7e_compress_blocks( numBlocks, dstBlock, srcBlock, &params );
        }
    }

    return true;
}


bool Compress_RGBASingleMip_To_BC_Alloc( uint8_t* srcMip, int width, int height, const BCCompressorSettings& settings, uint8_t** compressedOutput )
{
    const int blocksY = (height + 3) / 4;
    const int blocksX = (width + 3) / 4;
    const int totalBlocks = blocksX * blocksY;
    const int bytesPerBlock = NumBytesPerCompressedBlock( settings.format );
    *compressedOutput = static_cast<uint8_t*>( malloc( totalBlocks * bytesPerBlock ) );
    return Compress_RGBASingleMip_To_BC( srcMip, width, height, settings, *compressedOutput );
}


bool Compress_RGBAAllMips_To_BC( uint8_t* srcImage, int width, int height, const BCCompressorSettings& settings, uint8_t* compressedOutput )
{
    if ( !srcImage || !IsValidFormat( settings.format ) )
    {
        return false;
    }

    int bytesPerPixel = GetBCNumber( settings.format ) == 6 ? 4 : 8;
    int bytesPerBlock = NumBytesPerCompressedBlock( settings.format );
    do
    {
        Compress_RGBASingleMip_To_BC( srcImage, width, height, settings, compressedOutput );
        srcImage += width * height * bytesPerPixel;
        int blocksY = (height + 3) / 4;
        int blocksX = (width + 3) / 4;
        compressedOutput += blocksX * blocksY * bytesPerBlock;
        width = std::max( 1, width >> 1 );
        height = std::max( 1, height >> 1 );
    } while ( width != 1 || height != 1 );

    return true;
}


bool Compress_RGBAAllMips_To_BC_Alloc( uint8_t* srcImage, int width, int height, const BCCompressorSettings& settings, uint8_t** compressedOutput )
{
    if ( !srcImage || !IsValidFormat( settings.format ) )
    {
        return false;
    }

    size_t totalSize = 0;
    int bytesPerBlock = NumBytesPerCompressedBlock( settings.format );
    int w = width;
    int h = height;
    do
    {
        int blocksY = (height + 3) / 4;
        int blocksX = (width + 3) / 4;
        totalSize += blocksX * blocksY * bytesPerBlock;
        w = std::max( 1, w >> 1 );
        h = std::max( 1, h >> 1 );
    } while ( w != 1 || h != 1 );
    *compressedOutput = static_cast<uint8_t*>( malloc( totalSize ) );
    Compress_RGBAAllMips_To_BC( srcImage, width, height, settings, *compressedOutput );

    return true;
}