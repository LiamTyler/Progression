#pragma once

#include "core/pixel_formats.hpp"
#include "image.hpp"
#include "shared/assert.hpp"
#include "shared/color_spaces.hpp"
#include "shared/float_conversions.hpp"
#include <stdexcept>
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
    virtual vec4 Sample( vec2 uv, float mipLevel = 0 ) const = 0;
    virtual vec4 SampleDir( vec3 dir ) const                 = 0;
};

class Texture2D : public Texture
{
public:
    Texture2D() = default;
    Texture2D( int w, int h, int mipCount, PG::PixelFormat format, void* data )
    {
        pixelFormat = format;
        numChannels = PG::NumChannelsInPixelFromat( format );
        sRGB        = PG::PixelFormatIsSrgb( format );
        if ( PG::PixelFormatIsFloat16( format ) )
            pixelType = PixelType::FP16;
        else if ( PG::PixelFormatIsFloat32( format ) )
            pixelType = PixelType::FP16;
        else if ( PG::NumBytesPerChannel( format ) == 1 && PG::PixelFormatIsNormalized( format ) && PG::PixelFormatIsUnsigned( format ) )
            pixelType = PixelType::UNORM8;
        else
            throw std::runtime_error( "Invalid pixel format!" );

        mips.resize( mipCount );
        mipResolutions.resize( mipCount );
        int bytesPerPixel = PG::NumBytesPerPixel( format );
        size_t offset     = 0;
        for ( int i = 0; i < mipCount; ++i )
        {
            int mipSize       = w * h * bytesPerPixel;
            mips[i]           = std::make_unique<uint8_t[]>( mipSize );
            mipResolutions[i] = { w, h };
            memcpy( mips[i].get(), reinterpret_cast<char*>( data ) + offset, mipSize );
            w >>= 1;
            h >>= 1;
            offset += mipSize;
        }
    }

    template <typename T = uint8_t>
    T* Raw( int mipLevel )
    {
        return reinterpret_cast<T*>( mips[mipLevel].get() );
    }

    template <typename T = uint8_t>
    const T* Raw( int mipLevel ) const
    {
        return reinterpret_cast<const T*>( mips[mipLevel].get() );
    }

    vec4 Fetch( int row, int col, int mipLevel ) const
    {
        const int width  = mipResolutions[mipLevel].x;
        const int height = mipResolutions[mipLevel].y;
        if ( clampU )
        {
            col = std::clamp( col, 0, width - 1 );
        }
        else
        {
            if ( col < 0 )
                col += width;
            else if ( col >= width )
                col -= width;
        }
        if ( clampV )
        {
            row = std::clamp( row, 0, height - 1 );
        }
        else
        {
            if ( row < 0 )
                row += height;
            else if ( row >= height )
                row -= height;
        }

        int pixelOffset = numChannels * ( row * width + col );
        vec4 ret;
        for ( int channel = 0; channel < numChannels; ++channel )
        {
            switch ( pixelType )
            {
            case PixelType::UNORM8: ret[channel] = UNormByteToFloat( Raw<uint8_t>( mipLevel )[pixelOffset + channel] ); break;
            case PixelType::FP16: ret[channel] = Float16ToFloat32( Raw<uint16_t>( mipLevel )[pixelOffset + channel] ); break;
            case PixelType::FP32: ret[channel] = Raw<float>( mipLevel )[pixelOffset + channel]; break;
            }
        }

        if ( sRGB )
        {
            ret = PG::GammaSRGBToLinear( ret );
        }

        return ret;
    }

    vec4 Bilerp( vec2 uv, int mipLevel ) const
    {
        if ( clampU )
            uv.x = Saturate( uv.x );
        else
            uv.x -= std::floor( uv.x );
        if ( clampV )
            uv.y = Saturate( uv.y );
        else
            uv.y -= std::floor( uv.y );

        // subtract 0.5 to account for pixel centers
        uv         = uv * vec2( mipResolutions[mipLevel] ) - vec2( 0.5f );
        vec2 start = floor( uv );
        int col    = static_cast<int>( start.x );
        int row    = static_cast<int>( start.y );

        vec2 d          = uv - start;
        const float w00 = ( 1.0f - d.x ) * ( 1.0f - d.y );
        const float w01 = d.x * ( 1.0f - d.y );
        const float w10 = ( 1.0f - d.x ) * d.y;
        const float w11 = d.x * d.y;

        vec4 p00 = Fetch( row, col, mipLevel );
        vec4 p01 = Fetch( row, col + 1, mipLevel );
        vec4 p10 = Fetch( row + 1, col, mipLevel );
        vec4 p11 = Fetch( row + 1, col + 1, mipLevel );

        vec4 ret( 0, 0, 0, 1 );
        for ( int i = 0; i < numChannels; ++i )
        {
            ret[i] = w00 * p00[i] + w01 * p01[i] + w10 * p10[i] + w11 * p11[i];
        }

        return ret;
    }

    vec4 Sample( vec2 uv, float mipLevel = 0 ) const override
    {
        int mipCount = static_cast<int>( mips.size() );
        int firstMip = Min<int>( mipCount - 1, Max( 0, (int)std::floor( mipLevel ) ) );
        int nextMip  = Min<int>( mipCount - 1, Max( 0, firstMip + 1 ) );
        if ( firstMip == nextMip )
        {
            return Bilerp( uv, firstMip );
        }
        else
        {
            float lerpFactor = mipLevel - firstMip;
            return ( 1.0f - lerpFactor ) * Bilerp( uv, firstMip ) + lerpFactor * Bilerp( uv, nextMip );
        }
    }

    vec4 SampleDir( vec3 dir ) const override
    {
        float lon = std::atan2( dir.x, dir.y );
        float lat = std::atan2( dir.z, Length( vec2( dir ) ) );
        vec2 uv   = vec2( 0.5 * ( lon / PI + 1 ), lat / PI + 0.5 );
        return Sample( uv );
    }

    PG::PixelFormat pixelFormat = PG::PixelFormat::INVALID;
    bool clampU                 = false;
    bool clampV                 = false;
    std::vector<u16vec2> mipResolutions;
    std::vector<std::unique_ptr<uint8_t[]>> mips;

    // variables below are just cached, and can be inferred from pixelFormat
    int numChannels;
    bool sRGB;
    enum class PixelType
    {
        UNORM8,
        FP16,
        FP32
    };
    PixelType pixelType;
};

class TextureCubeMap : public Texture
{
public:
    TextureCubeMap( int w, int h, int mipLevels, PG::PixelFormat format, void* facePixelData[6] )
    {
        for ( int face = 0; face < 6; ++face )
        {
            faces[face] = Texture2D( w, h, mipLevels, format, facePixelData[face] );
        }
    }

    vec4 Sample( vec2 uv, float mipLevel = 0 ) const override
    {
        PG_ASSERT( false, "Invalid for cubemaps. Use SampleDir" );
        return vec4( 0 );
    }

    // assumes dir (v) is normalized, and that +x = right, +y = up, +z = forward
    vec4 SampleDir( vec3 v ) const override
    {
        vec3 vAbs = abs( v );
        float ma;
        vec2 uv;
        int faceIndex;
        if ( vAbs.z >= vAbs.x && vAbs.z >= vAbs.y )
        {
            faceIndex = v.z < 0.0f ? 5 : 4;
            ma        = 0.5f / vAbs.z;
            uv        = vec2( v.z < 0.0f ? -v.x : v.x, -v.y );
        }
        else if ( vAbs.y >= vAbs.x )
        {
            faceIndex = v.y < 0.0f ? 3 : 2;
            ma        = 0.5f / vAbs.y;
            uv        = vec2( v.x, v.y < 0.0f ? -v.z : v.z );
        }
        else
        {
            faceIndex = v.x < 0.0 ? 1 : 0;
            ma        = 0.5f / vAbs.x;
            uv        = vec2( v.x < 0.0f ? v.z : -v.z, -v.y );
        }
        uv = uv * ma + vec2( 0.5f );

        return faces[faceIndex].Sample( uv );
    }

    Texture2D faces[6]; // face order: +x,-x,+y,-y,+z,-z
private:
    vec2 halfPixelDim;
};

using TextureHandle                            = uint16_t;
constexpr TextureHandle TEXTURE_HANDLE_INVALID = 0;

TextureHandle LoadTextureFromGfxImage( PG::GfxImage* image );

Texture* GetTex( TextureHandle handle );

} // namespace PT
