#pragma once

#include "image.hpp"
#include "shared/color_spaces.hpp"
#include "shared/float_conversions.hpp"
#include <vector>

namespace PG
{
    struct GfxImage;
} // namespace PG

namespace PT
{

class Texture
{
public:
    virtual ~Texture() {}
    virtual glm::vec4 Sample( glm::vec2 uv ) const = 0;
};

template <typename T, int numChannels, bool sRGB, bool clampU = false, bool clampV = false>
class Texture2D : public Texture
{
public:
    static_assert( numChannels >= 1 && numChannels <= 4 );
    Texture2D( int w, int h, void* data ) : width( w ), height( h )
    {
        pixels = std::make_shared<T[]>( w * h * numChannels );
        halfPixelDim = glm::vec2( 0.5f ) / glm::vec2( width, height );
        memcpy( pixels.get(), data, w * h * numChannels * sizeof( T ) );
    }


    glm::vec4 Fetch( int row, int col ) const
    {
        if constexpr ( clampU )
        {
            col = std::clamp( col, 0, width - 1 );
        }
        else
        {
            if ( col < 0 ) col += width;
            else if ( col >= width ) col -= width;
        }
        if constexpr ( clampV )
        {
            row = std::clamp( row, 0, height - 1 );
        }
        else
        {
            if ( row < 0 ) row += height;
            else if ( row >= height ) row -= height;
        }

        int pixelOffset = numChannels * (row * width + col);
        glm::vec4 ret;
        for ( int channel = 0; channel < numChannels; ++channel )
        {
            if constexpr ( std::is_same_v<T,float> )
            {
                ret[channel] = pixels[pixelOffset + channel];
            }
            else if constexpr ( std::is_same_v<T,float16> )
            {
                ret[channel] = Float16ToFloat32( pixels[pixelOffset + channel] );
            }
            else if constexpr ( std::is_same_v<T,uint8_t> )
            {
                ret[channel] = UNormByteToFloat( pixels[pixelOffset + channel] );
            }

            // don't apply to alpha channel
            if ( sRGB && channel != 3 ) ret[channel] = PG::GammaSRGBToLinear( ret[channel] );
        }

        return ret;
    }


    glm::vec4 Sample( glm::vec2 uv ) const override
    {
        if constexpr ( clampU ) uv.x = std::clamp( uv.x, 0.0f, 1.0f );
        else uv.x -= std::floor( uv.x );
        if constexpr ( clampV ) uv.y = std::clamp( uv.y, 0.0f, 1.0f );
        else uv.y -= std::floor( uv.y );

        // subtract 0.5 to account for pixel centers
        uv = uv * glm::vec2( width, height ) - glm::vec2( 0.5f );
        glm::vec2 start = glm::floor( uv );
        int col = static_cast<int>( start.x );
        int row = static_cast<int>( start.y );
        
        glm::vec2 d = uv - start;
        const float w00 = (1.0f - d.x) * (1.0f - d.y);
		const float w01 = d.x * (1.0f - d.y);
		const float w10 = (1.0f - d.x) * d.y;
		const float w11 = d.x * d.y;
        
        glm::vec4 p00 = Fetch( row, col );
        glm::vec4 p01 = Fetch( row, col + 1 );
        glm::vec4 p10 = Fetch( row + 1, col );
        glm::vec4 p11 = Fetch( row + 1, col + 1 );
        
        glm::vec4 ret;
        for ( int i = 0; i < numChannels; ++i )
        {
            ret[i] = w00 * p00[i] + w01 * p01[i] + w10 * p10[i] + w11 * p11[i];
        }

        if constexpr ( numChannels < 2 ) ret.g = 0.0f;
        if constexpr ( numChannels < 3 ) ret.b = 0.0f;
        if constexpr ( numChannels < 4 ) ret.a = 1.0f;

        return ret;
    }

    int width;
    int height;
    std::shared_ptr<T[]> pixels;
private:
    glm::vec2 halfPixelDim;
};

using TextureHandle = uint16_t;
constexpr TextureHandle TEXTURE_HANDLE_INVALID = 0;

TextureHandle LoadTextureFromGfxImage( PG::GfxImage* image );

Texture* GetTex( TextureHandle handle );

} // namespace PT