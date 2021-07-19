#include "core/image_calculate_size.hpp"
#include "core/assert.hpp"
#include <algorithm>
#include <cmath>

namespace PG
{

    uint32_t CalculateNumMips( uint32_t width, uint32_t height )
    {
        uint32_t largestDim = std::max( width, height );
        if ( largestDim == 0 )
        {
            return 0;
        }

        return 1 + static_cast< uint32_t >( std::log2f( static_cast< float >( largestDim ) ) );
    }


    size_t CalculateTotalFaceSizeWithMips( uint32_t width, uint32_t height, PixelFormat format, uint32_t numMips )
    {
        PG_ASSERT( width > 0 && height > 0 );
        PG_ASSERT( format != PixelFormat::INVALID );
        PG_ASSERT( !PixelFormatIsCompressed( format ), "compressed format not supported yet" );
        uint32_t w = width;
        uint32_t h = height;
        if ( numMips == 0 )
        {
            numMips = CalculateNumMips( w, h );
        }
        int bytesPerPixel = NumBytesPerPixel( format );
        size_t currentSize = 0;
        for ( uint32_t mipLevel = 0; mipLevel < numMips; ++mipLevel )
        {
            currentSize += w * h * bytesPerPixel;
            w = std::max( 1u, w >> 1 );
            h = std::max( 1u, h >> 1 );
        }

        return currentSize;
    }


    size_t CalculateTotalImageBytes( PixelFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint32_t mipLevels )
    {
        size_t totalBytes = depth * arrayLayers * CalculateTotalFaceSizeWithMips( width, height, format, mipLevels );

        return totalBytes;
    }

} // namespace PG
