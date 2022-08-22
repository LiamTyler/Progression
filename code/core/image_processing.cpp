#include "image_processing.hpp"
#include "shared/assert.hpp"
#include "shared/logger.hpp"
#include "shared/color_spaces.hpp"
#include <stdexcept>

namespace PG
{

FloatImage CompositeImage( const CompositeImageInput& input )
{
    PG_ASSERT( input.compositeType == CompositeType::REMAP );

    // validate
    if ( input.outputColorSpace != ColorSpace::SRGB && input.outputColorSpace != ColorSpace::LINEAR )
        throw std::runtime_error( "CompositeImage: output color space must be set to SRGB or LINEAR" );
    if ( input.sourceImages.size() == 0 )
        throw std::runtime_error( "CompositeImage: no source images specified" );
    if ( input.sourceImages.size() > 4 )
        throw std::runtime_error( "CompositeImage: too many source images. Only specify up to 4" );

    uint32_t numOutputChannels = 0;
    FloatImage sourceImages[4];
    ColorSpace sourceColorSpaces[4];
    uint32_t width = 0;
    uint32_t height = 0;
    for ( size_t i = 0; i < input.sourceImages.size(); ++i )
    {
        if ( !sourceImages[i].Load( input.sourceImages[i].filename ) )
        {
            throw std::runtime_error( "Failed to load composite source image" );
        }
        sourceColorSpaces[i] = input.sourceImages[i].colorSpace;
        if ( sourceColorSpaces[i] == ColorSpace::INFER )
        {
            sourceColorSpaces[i] = sourceImages[i].numChannels > 2 ? ColorSpace::SRGB : ColorSpace::LINEAR;
        }

        width = std::max( width, sourceImages[i].width );
        height = std::max( height, sourceImages[i].height );

        for ( const Remap& remap : input.sourceImages[i].remaps )
        {
            numOutputChannels = std::max<uint32_t>( numOutputChannels, Underlying( remap.to ) + 1 );
        }
    }

    ColorSpace outputColorSpace = input.outputColorSpace;
    FloatImage outputImg( width, height, numOutputChannels );
    for ( size_t i = 0; i < input.sourceImages.size(); ++i )
    {
        FloatImage srcImage = sourceImages[i].Resize( width, height );
        bool convertToLinear = sourceColorSpaces[i] == ColorSpace::SRGB && outputColorSpace == ColorSpace::LINEAR;
        bool convertToSRGB = sourceColorSpaces[i] == ColorSpace::LINEAR && outputColorSpace == ColorSpace::SRGB;
        for ( uint32_t pixelIndex = 0; pixelIndex < width * height; ++pixelIndex )
        {
            glm::vec4 pixel = srcImage.GetFloat4( pixelIndex );
            

            for ( const Remap& remap : input.sourceImages[i].remaps )
            {
                float x = pixel[Underlying( remap.from )];
                if ( remap.to != Channel::A )
                {
                    float x = pixel[Underlying( remap.from )];
                    if ( convertToLinear ) x = PG::GammaSRGBToLinear( x );
                    else if ( convertToSRGB ) x = PG::LinearToGammaSRGB( x );
                }
                outputImg.data[outputImg.numChannels * pixelIndex + Underlying( remap.to )] = x;
            }
        }
    }


    return outputImg;

}

} // namespace PG
