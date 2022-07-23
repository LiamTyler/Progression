#include "bc_compression.hpp"
#include "bc7enc_rdo/rgbcx.h"
#include "bc7enc_rdo/bc7e_ispc.h"
#include "compressonator/cmp_core/source/cmp_core.h"
#include "image.hpp"
#include "shared/assert.hpp"
#include "shared/logger.hpp"


int GetBCNumber( RawImage2D::Format format )
{
    int8_t mapping[] =
    {
        1, // BC1_UNORM,
        2, // BC2_UNORM,
        3, // BC3_UNORM,
        4, // BC4_UNORM,
        4, // BC4_SNORM,
        5, // BC5_UNORM,
        5, // BC5_SNORM,
        6, // BC6H_U16F,
        6, // BC6H_S16F,
        7, // BC7_UNORM,
    };

    if ( static_cast<uint8_t>( format ) < static_cast<uint8_t>( RawImage2D::Format::BC1_UNORM ) ) return 0;

    return mapping[static_cast<uint8_t>( format ) - static_cast<uint8_t>( RawImage2D::Format::BC1_UNORM )];
}

/*
bool Compress_RGBASingleMip_To_BC( uint8_t* srcMip, int width, int height, const BCCompressorSettings& settings, uint8_t* compressedOutput )
{
    const int blocksY = (height + 3) / 4;
    const int blocksX = (width + 3) / 4;
    const int totalBlocks = blocksX * blocksY;
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
        std::vector<glm::u8vec4> blocks( totalBlocks * 16 );
        #pragma omp parallel for
        for ( int by = 0; by < blocksY; ++by )
	    {
		    for ( int bx = 0; bx < blocksX; ++bx )
		    {
                image.GetBlockClamped( bx, by, &blocks[16 * (by*blocksX + bx)] );
            }
        }
        
        const int blocksPerChunk = 64;
        const int chunks = (totalBlocks + blocksPerChunk - 1) / blocksPerChunk;
        #pragma omp parallel for
        for ( int chunk = 0; chunk < chunks; ++chunk )
	    {
            uint64_t* dstBlock = reinterpret_cast<uint64_t*>( compressedOutput + chunk * blocksPerChunk * bytesPerBlock );
            uint32_t numBlocks = std::min( blocksPerChunk, totalBlocks - chunk * blocksPerChunk );
            uint32_t* srcBlock = reinterpret_cast<uint32_t*>( &blocks[16 * (chunk * blocksPerChunk)] );
            ispc::bc7e_compress_blocks( numBlocks, dstBlock, srcBlock, &params );
        }
    }

    return true;
}
*/

void Compress_BC1( const RawImage2D& srcImage, const BCCompressorSettings& settings, RawImage2D& outputImg )
{
    const int blocksX = srcImage.BlocksX();
    const int blocksY = srcImage.BlocksY();
    const int bytesPerBlock = outputImg.BytesPerBlock();

    rgbcx::init();

    uint32_t qualityLevel = rgbcx::MIN_LEVEL;
    if ( settings.quality == CompressionQuality::MEDIUM ) qualityLevel = 10;
    else if ( settings.quality == CompressionQuality::HIGHEST ) qualityLevel = rgbcx::MAX_LEVEL;

    #pragma omp parallel for
	for ( int by = 0; by < blocksY; ++by )
	{
		for ( int bx = 0; bx < blocksX; ++bx )
		{
            uint8_t src[64];
            srcImage.GetBlockClamped8Bit( bx, by, src );
            uint8_t* dst = &outputImg.data[bytesPerBlock * (by * blocksX + bx)];
            rgbcx::encode_bc1( qualityLevel, dst, src, true, true );
        }
    }
}

RawImage2D CompressToBC( RawImage2D image, const BCCompressorSettings& settings )
{
    RawImage2D compressedImg( image.width, image.height, settings.format );

    bool error = false;
    switch ( settings.format )
    {
    case RawImage2D::Format::BC1_UNORM:
        Compress_BC1( image, settings, compressedImg );
        break;
    case RawImage2D::Format::BC2_UNORM:
        LOG_ERR( "CompressToBC: BC2 is not suppported, please use BC3 instead" );
        error = true;
        break;
    default:
        LOG_ERR( "CompressToBC: trying to compress to a format (%u) that is not a BC format", (uint32_t) settings.format );
        error = true;
    }

    if ( error )
    {
        compressedImg.data = nullptr;
    }

    return compressedImg;
}