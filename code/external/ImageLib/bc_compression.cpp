#include "bc_compression.hpp"
#include "bc7enc_rdo/rgbcx.h"
#include "bc7enc_rdo/bc7e_ispc.h"
#include "compressonator/cmp_core/source/cmp_core.h"
#include "shared/assert.hpp"
#include "shared/logger.hpp"
#include <vector>


template <int FORMAT>
void Compress_BC_12345( const RawImage2D& srcImage, const BCCompressorSettings& settings, RawImage2D& outputImg )
{
    static_assert( Underlying( CompressionQuality::COUNT ) == 3, "Update the quality level integers below" );
    const int blocksX = srcImage.BlocksX();
    const int blocksY = srcImage.BlocksY();
    const int bytesPerBlock = outputImg.BytesPerBlock();

    rgbcx::init();
    const uint32_t bc1Level = 0 + 9 * Underlying( settings.quality );
    const uint32_t bc4Level = 1 + 5 * Underlying( settings.quality );
    const bool isLowQuality = settings.quality == CompressionQuality::LOWEST;

    #pragma omp parallel for
	for ( int by = 0; by < blocksY; ++by )
	{
		for ( int bx = 0; bx < blocksX; ++bx )
		{
            uint8_t src[64];
            srcImage.GetBlockClamped8Bit( bx, by, src );
            uint8_t* dst = &outputImg.data[bytesPerBlock * (by * blocksX + bx)];

            if constexpr ( FORMAT == Underlying( ImageFormat::BC1_UNORM ) )
            {
                rgbcx::encode_bc1( bc1Level, dst, src, true, true );
            }
            else if constexpr ( FORMAT == Underlying( ImageFormat::BC3_UNORM ) )
            {
                if ( isLowQuality ) rgbcx::encode_bc3( bc1Level, dst, src );
                else rgbcx::encode_bc3_hq( bc1Level, dst, src, bc4Level );
            }
            else if constexpr ( FORMAT == Underlying( ImageFormat::BC4_UNORM ) || FORMAT == Underlying( ImageFormat::BC4_SNORM ) )
            {
                if ( isLowQuality ) rgbcx::encode_bc4( dst, &src[settings.bc4SourceChannel] );
                else rgbcx::encode_bc4_hq( dst, &src[settings.bc4SourceChannel], 4, bc4Level );
            }
            else if constexpr ( FORMAT == Underlying( ImageFormat::BC5_UNORM ) || FORMAT == Underlying( ImageFormat::BC5_SNORM ) )
            {
                if ( isLowQuality ) rgbcx::encode_bc5( dst, src, settings.bc5SourceChannel1, settings.bc5SourceChannel2 );
                rgbcx::encode_bc5_hq( dst, src, settings.bc5SourceChannel1, settings.bc5SourceChannel2, 4, bc4Level );
            }
        }
    }
}


void Compress_BC_6( const RawImage2D& srcImage, const BCCompressorSettings& settings, RawImage2D& outputImg )
{
    const int blocksX = srcImage.BlocksX();
    const int blocksY = srcImage.BlocksY();
    const int bytesPerBlock = outputImg.BytesPerBlock();

    void* options = nullptr;
    CreateOptionsBC6( &options );
    SetQualityBC6( options, (float)settings.quality / (float)CompressionQuality::HIGHEST ); // quality is continuous in [0,1], the higher the better
    bool isSigned = settings.format == ImageFormat::BC6H_S16F;
    SetSignedBC6( options, isSigned );

    #pragma omp parallel for
	for ( int by = 0; by < blocksY; ++by )
	{
		for ( int bx = 0; bx < blocksX; ++bx )
		{
            float16 blockSrc[48];
            srcImage.GetBlockClamped16F( bx, by, blockSrc );

            // Note1: Compressenator doesn't seem to do any work checks to do any clamping for negative numbers, so do it here
            // Note2: It's important to not allow for -0.0 with Compressenator's encoder, because it casts the uint16_t directly to a float.
            // This means that it expects nearby input values to have nearby uint16_t values (at least for unsigned BC6), which is not true for -0.0 and 0.0.
            // So an easy fix is to just not allow -0.0 (which is 32768 as a uint16_t) in the fp16 input to the BC6 compressor
            if ( !isSigned )
            {
                for ( int i = 0; i < 48; ++i )
                {
                    if ( blockSrc[i] & 0x8000 ) blockSrc[i] = 0;
                }
            }
            else
            {
                for ( int i = 0; i < 48; ++i )
                {
                    if ( blockSrc[i] == 0x8000 ) blockSrc[i] = 0;
                }
            }
            uint8_t* blockDst = outputImg.Raw() + bytesPerBlock * (by * blocksX + bx);
            CompressBlockBC6( (uint16_t*)blockSrc, 12, blockDst, options );
        }
    }
    DestroyOptionsBC6( options );
}


void Compress_BC_7( const RawImage2D& srcImage, const BCCompressorSettings& settings, RawImage2D& outputImg )
{
    const int blocksX = srcImage.BlocksX();
    const int blocksY = srcImage.BlocksY();
    const int totalBlocks = blocksX * blocksY;
    const int bytesPerBlock = 16;

    ispc::bc7e_compress_block_params params;
    ispc::bc7e_compress_block_init();
    if ( settings.quality == CompressionQuality::LOWEST ) ispc::bc7e_compress_block_params_init_ultrafast( &params, true );
    else if ( settings.quality == CompressionQuality::MEDIUM ) ispc::bc7e_compress_block_params_init_basic( &params, true );
    else ispc::bc7e_compress_block_params_init_slowest( &params, true );

    struct Block
    {
        uint8_t data[64];
    };

    // bc7e wants an array of 4x4 RGBA blocks, at least 32-64 at a time if possible
    std::vector<Block> blocks( totalBlocks );

    #pragma omp parallel for
    for ( int by = 0; by < blocksY; ++by )
	{
		for ( int bx = 0; bx < blocksX; ++bx )
		{
            srcImage.GetBlockClamped8Bit( bx, by, blocks[by*blocksX + bx].data );
        }
    }
        
    const int blocksPerChunk = 64;
    const int chunks = (totalBlocks + blocksPerChunk - 1) / blocksPerChunk;
    #pragma omp parallel for
    for ( int chunk = 0; chunk < chunks; ++chunk )
	{
        uint64_t* dstBlock = reinterpret_cast<uint64_t*>( outputImg.Raw() + chunk * blocksPerChunk * bytesPerBlock );
        uint32_t numBlocks = std::min( blocksPerChunk, totalBlocks - chunk * blocksPerChunk );
        uint32_t* srcBlock = reinterpret_cast<uint32_t*>( blocks[chunk * blocksPerChunk].data );
        ispc::bc7e_compress_blocks( numBlocks, dstBlock, srcBlock, &params );
    }
}


RawImage2D CompressToBC( const RawImage2D& image, const BCCompressorSettings& settings )
{
    RawImage2D compressedImg( image.width, image.height, settings.format );

    bool error = false;
    switch ( settings.format )
    {
    case ImageFormat::BC1_UNORM:
        Compress_BC_12345<Underlying( ImageFormat::BC1_UNORM )>( image, settings, compressedImg );
        break;
    case ImageFormat::BC2_UNORM:
        LOG_ERR( "CompressToBC: BC2 is not suppported, please use BC3 instead" );
        error = true;
        break;
    case ImageFormat::BC3_UNORM:
        Compress_BC_12345<Underlying( ImageFormat::BC3_UNORM )>( image, settings, compressedImg );
        break;
    case ImageFormat::BC4_UNORM:
        Compress_BC_12345<Underlying( ImageFormat::BC4_UNORM )>( image, settings, compressedImg );
        break;
    case ImageFormat::BC4_SNORM:
        Compress_BC_12345<Underlying( ImageFormat::BC4_SNORM )>( image, settings, compressedImg );
        break;
    case ImageFormat::BC5_UNORM:
        Compress_BC_12345<Underlying( ImageFormat::BC5_UNORM )>( image, settings, compressedImg );
        break;
    case ImageFormat::BC5_SNORM:
        Compress_BC_12345<Underlying( ImageFormat::BC5_SNORM )>( image, settings, compressedImg );
        break;
    case ImageFormat::BC6H_U16F:
    case ImageFormat::BC6H_S16F:
        Compress_BC_6( image, settings, compressedImg );
        break;
    case ImageFormat::BC7_UNORM:
        Compress_BC_7( image, settings, compressedImg );
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


std::vector<RawImage2D> CompressToBC( const std::vector<RawImage2D>& images, const BCCompressorSettings& settings )
{
    std::vector<RawImage2D> outputImages( images.size() );
    for ( size_t i = 0; i < images.size(); ++i )
    {
        outputImages[i] = CompressToBC( images[i], settings );
    }

    return outputImages;
}