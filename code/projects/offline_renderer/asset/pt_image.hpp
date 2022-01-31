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

        // assuming uv = (0,0) is not the center of the first pixel, but rather the top left corner of it
        // Ex: if a 2x2 texture is sampled at (0.25, 0.25), then only the first pixel should have any weight during bilinear filtering
        int col0, col1;
        float colLerp;
        float centeredU = width * (uv.x - halfPixelDim.x);
        if ( centeredU >= 0.0f ) [[likely]]
        {
            col0 = static_cast<int>( centeredU );
            col1 = col0 + 1;
            if ( col1 == width ) [[unlikely]]
                col1 = 0;
            colLerp = centeredU - col0;
        }
        else
        {
            col0 = width - 1;
            col1 = 0;
            colLerp = 1 + centeredU;
        }

        int row0, row1;
        float rowLerp;
        float centeredV = height * (uv.y - halfPixelDim.y);
        if ( centeredV >= 0.0f ) [[likely]]
        {
            row0 = static_cast<int>( centeredV );
            row1 = row0 + 1;
            if ( row1 == height ) [[unlikely]]
                row1 = 0;
            rowLerp = centeredV - row0;
        }
        else
        {
            row0 = height - 1;
            row1 = 0;
            rowLerp = 1 + centeredV;
        }
        
        glm::vec4 p00 = Fetch( row0, col0 );
        glm::vec4 p01 = Fetch( row0, col1 );
        glm::vec4 p10 = Fetch( row1, col0 );
        glm::vec4 p11 = Fetch( row1, col1 );

        glm::vec4 p0 = (1.0f - colLerp) * p00 + colLerp * p01;
        glm::vec4 p1 = (1.0f - colLerp) * p10 + colLerp * p11;
        
        glm::vec4 ret = (1.0f - rowLerp) * p0 + rowLerp * p1;
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