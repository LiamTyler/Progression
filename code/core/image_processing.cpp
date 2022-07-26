#include "image_processing.hpp"
#include "shared/assert.hpp"
#include "shared/logger.hpp"
#include <stdexcept>

namespace PG
{

FloatImage CompositeImage( const CompositeImageInput& input )
{
    PG_ASSERT( input.compositeType == CompositeType::REMAP );

    FloatImage sourceImages[4];
    uint32_t width = 0;
    uint32_t height = 0;
    for ( int i = 0; i < 4; ++i )
    {
        if ( !sourceImages[i].Load( input.sourceImages[i] ) )
        {
            throw std::runtime_error( "Failed to load composite source image" );
        }
        width = std::max( width, sourceImages[i].width );
        height = std::max( height, sourceImages[i].height );
    }

    int numChannels = 0;
    for ( const OutputChannelInfo& remapInfo : input.outputChannelInfos )
    {
        numChannels = std::max( numChannels, remapInfo.outputChannelIndex );
    }
    FloatImage outputImg( width, height, numChannels );

    return outputImg;

}

} // namespace PG
