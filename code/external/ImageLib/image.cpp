#include "image.hpp"
#include "bc_compression.hpp"
#include "shared/assert.hpp"
#include "shared/float_conversions.hpp"
#include "shared/logger.hpp"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb/stb_image_resize.h"


static glm::u8vec4  DEFAULT_PIXEL_BYTE    = glm::u8vec4( 0, 0, 0, 255 );
static glm::u16vec4 DEFAULT_PIXEL_SHORT   = glm::u16vec4( 0, 0, 0, USHRT_MAX );
static glm::u16vec4 DEFAULT_PIXEL_FLOAT16 = glm::u16vec4( 0, 0, 0, Float32ToFloat16( 1.0f ) );
static glm::vec4    DEFAULT_PIXEL_FLOAT32 = glm::vec4( 0, 0, 0, 1 );


uint8_t* RawImage2D::GetCompressedBlock( int blockX, int blockY )
{
    uint32_t numBlocksX = (width + 3) / 4;
    size_t offset = 16 * BitsPerPixel() / 8 * (blockY * numBlocksX + blockX);
    return &data[offset];
}


template <typename T, std::underlying_type_t<ImageFormat> baseFormat>
void GetBlockClamped8BitConvert( uint32_t blockX, uint32_t blockY, uint32_t width, uint32_t height, uint32_t numChannels, uint8_t* outputRGBA, const T* input )
{
    for ( uint32_t r = 0; r < 4; ++r )
    {
        uint32_t row = std::min( height - 1, 4 * blockY + r );
        for ( uint32_t c = 0; c < 4; ++c )
        {
            uint32_t col = std::min( width - 1, 4 * blockX + c );

            uint32_t srcOffset = numChannels * (row * width + col);
            uint32_t dstOffset = 4 * (r * 4 + c);
            for ( uint32_t channel = 0; channel < 4; ++channel )
            {
                if ( channel < numChannels )
                {
                    if constexpr ( baseFormat == Underlying( ImageFormat::R8_UNORM ) )
                    {
                        outputRGBA[dstOffset + channel] = input[srcOffset + channel];
                    }
                    else if constexpr ( baseFormat == Underlying( ImageFormat::R16_UNORM ) )
                    {
                        outputRGBA[dstOffset + channel] = UNormFloatToByte( UNorm16ToFloat( input[srcOffset + channel] ) );
                    }
                    else if constexpr ( baseFormat == Underlying( ImageFormat::R16_FLOAT ) )
                    {
                        outputRGBA[dstOffset + channel] = UNormFloatToByte( Float16ToFloat32( input[srcOffset + channel] ) );
                    }
                    else
                    {
                        outputRGBA[dstOffset + channel] = UNormFloatToByte( input[srcOffset + channel] );
                    }
                }
                else
                {
                    outputRGBA[dstOffset + channel] = DEFAULT_PIXEL_BYTE[channel];
                }
            }
        }
    }
}


void RawImage2D::GetBlockClamped8Bit( int blockX, int blockY, uint8_t* outputRGBA ) const
{
    uint32_t numChannels = NumChannels();
    if ( IsFormat8BitUnorm( format ) )
    {
        GetBlockClamped8BitConvert<uint8_t, Underlying( ImageFormat::R8_UNORM )>( (uint32_t)blockX, (uint32_t)blockY, width, height, numChannels, outputRGBA, Raw<uint8_t>() );
    }
    else if ( IsFormat16BitUnorm( format ) )
    {
        GetBlockClamped8BitConvert<uint16_t, Underlying( ImageFormat::R16_UNORM )>( (uint32_t)blockX, (uint32_t)blockY, width, height, numChannels, outputRGBA, Raw<uint16_t>() );
    }
    else if ( IsFormat16BitFloat( format ) )
    {
        GetBlockClamped8BitConvert<float16, Underlying( ImageFormat::R16_FLOAT )>( (uint32_t)blockX, (uint32_t)blockY, width, height, numChannels, outputRGBA, Raw<float16>() );
    }
    else if ( IsFormat32BitFloat( format ) )
    {
        GetBlockClamped8BitConvert<float, Underlying( ImageFormat::R32_FLOAT )>( (uint32_t)blockX, (uint32_t)blockY, width, height, numChannels, outputRGBA, Raw<float>() );
    }
    else
    {
        memset( outputRGBA, 0, 64 );
    }
}


template <typename T, std::underlying_type_t<ImageFormat> baseFormat>
void GetBlockClamped16FConvert( uint32_t blockX, uint32_t blockY, uint32_t width, uint32_t height, uint32_t numChannels, float16* outputRGB, const T* input )
{
    for ( uint32_t r = 0; r < 4; ++r )
    {
        uint32_t row = std::min( height - 1, 4 * blockY + r );
        for ( uint32_t c = 0; c < 4; ++c )
        {
            uint32_t col = std::min( width - 1, 4 * blockX + c );

            uint32_t srcOffset = numChannels * (row * width + col);
            uint32_t dstOffset = 3 * (r * 4 + c);
            for ( uint32_t channel = 0; channel < 3; ++channel )
            {
                if ( channel < numChannels )
                {
                    if constexpr ( baseFormat == Underlying( ImageFormat::R8_UNORM ) )
                    {
                        outputRGB[dstOffset + channel] = Float32ToFloat16( UNormByteToFloat( input[srcOffset + channel] ) );
                    }
                    else if constexpr ( baseFormat == Underlying( ImageFormat::R16_UNORM ) )
                    {
                        outputRGB[dstOffset + channel] = Float32ToFloat16( UNorm16ToFloat( input[srcOffset + channel] ) );
                    }
                    else if constexpr ( baseFormat == Underlying( ImageFormat::R16_FLOAT ) )
                    {
                        outputRGB[dstOffset + channel] = input[srcOffset + channel];
                    }
                    else
                    {
                        outputRGB[dstOffset + channel] = Float32ToFloat16( input[srcOffset + channel] );
                    }
                }
                else
                {
                    outputRGB[dstOffset + channel] = DEFAULT_PIXEL_FLOAT16[channel];
                }
            }
        }
    }
}


void RawImage2D::GetBlockClamped16F( int blockX, int blockY, float16* outputRGB ) const
{
    uint32_t numChannels = NumChannels();
    if ( IsFormat8BitUnorm( format ) )
    {
        GetBlockClamped16FConvert<uint8_t, Underlying( ImageFormat::R8_UNORM )>( (uint32_t)blockX, (uint32_t)blockY, width, height, numChannels, outputRGB, Raw<uint8_t>() );
    }
    else if ( IsFormat16BitUnorm( format ) )
    {
        GetBlockClamped16FConvert<uint16_t, Underlying( ImageFormat::R16_UNORM )>( (uint32_t)blockX, (uint32_t)blockY, width, height, numChannels, outputRGB, Raw<uint16_t>() );
    }
    else if ( IsFormat16BitFloat( format ) )
    {
        GetBlockClamped16FConvert<float16, Underlying( ImageFormat::R16_FLOAT )>( (uint32_t)blockX, (uint32_t)blockY, width, height, numChannels, outputRGB, Raw<float16>() );
    }
    else if ( IsFormat32BitFloat( format ) )
    {
        GetBlockClamped16FConvert<float, Underlying( ImageFormat::R32_FLOAT )>( (uint32_t)blockX, (uint32_t)blockY, width, height, numChannels, outputRGB, Raw<float>() );
    }
    else
    {
        memset( outputRGB, 0, 96 );
    }
}


float RawImage2D::GetPixelAsFloat( int row, int col, int chan ) const
{
    uint32_t numChannels = NumChannels();
    if ( chan >= (int)numChannels )
    {
        return DEFAULT_PIXEL_FLOAT32[chan];
    
    }
    int index = numChannels * (row * width + col) + chan;

    if ( IsFormat8BitUnorm( format ) )
    {
        return UNormByteToFloat( Raw<uint8_t>()[index] );
    }
    else if ( IsFormat16BitUnorm( format ) )
    {
        return UNorm16ToFloat( Raw<uint16_t>()[index] );
    }
    else if ( IsFormat16BitFloat( format ) )
    {
        return Float16ToFloat32( Raw<float16>()[index] );
    }
    else if ( IsFormat32BitFloat( format ) )
    {
        return Raw<float>()[index];
    }

    return 0;
}


glm::vec4 RawImage2D::GetPixelAsFloat4( int row, int col ) const
{
    glm::vec4 pixel = DEFAULT_PIXEL_FLOAT32;
    uint32_t numChannels = NumChannels();
    uint32_t index = numChannels * (row * width + col);
    if ( IsFormat8BitUnorm( format ) )
    {
        for ( uint32_t chan = 0; chan < numChannels; ++chan )
            pixel[chan] = UNormByteToFloat( Raw<uint8_t>()[index + chan] );
    }
    else if ( IsFormat16BitUnorm( format ) )
    {
        for ( uint32_t chan = 0; chan < numChannels; ++chan )
            pixel[chan] = UNorm16ToFloat( Raw<uint16_t>()[index + chan] );
    }
    else if ( IsFormat16BitFloat( format ) )
    {
        for ( uint32_t chan = 0; chan < numChannels; ++chan )
            pixel[chan] = Float16ToFloat32( Raw<float16>()[index + chan] );
    }
    else if ( IsFormat32BitFloat( format ) )
    {
        for ( uint32_t chan = 0; chan < numChannels; ++chan )
            pixel[chan] = Raw<float>()[index + chan];
    }

    return pixel;
}


void RawImage2D::SetPixelFromFloat( int row, int col, int chan, float x )
{
    uint32_t numChannels = NumChannels();
    int index = numChannels * (row * width + col) + chan;
    if ( IsFormat8BitUnorm( format ) )
    {
        Raw<uint8_t>()[index] = UNormFloatToByte( x );
    }
    else if ( IsFormat16BitUnorm( format ) )
    {
        Raw<uint16_t>()[index] = FloatToUNorm16( x );
    }
    else if ( IsFormat16BitFloat( format ) )
    {
        Raw<float16>()[index] = Float32ToFloat16( x );
    }
    else if ( IsFormat32BitFloat( format ) )
    {
        Raw<float>()[index] = x;
    }
}


void RawImage2D::SetPixelFromFloat4( int row, int col, glm::vec4 pixel )
{
    uint32_t numChannels = NumChannels();
    uint32_t index = numChannels * (row * width + col);
    if ( IsFormat8BitUnorm( format ) )
    {
        for ( uint32_t chan = 0; chan < numChannels; ++chan )
            Raw<uint8_t>()[index + chan] = UNormFloatToByte( pixel[chan] );
    }
    else if ( IsFormat16BitUnorm( format ) )
    {
        for ( uint32_t chan = 0; chan < numChannels; ++chan )
            Raw<uint16_t>()[index + chan] = FloatToUNorm16( pixel[chan] );
    }
    else if ( IsFormat16BitFloat( format ) )
    {
        for ( uint32_t chan = 0; chan < numChannels; ++chan )
            Raw<float16>()[index + chan] = Float32ToFloat16( pixel[chan] );
    }
    else if ( IsFormat32BitFloat( format ) )
    {
        for ( uint32_t chan = 0; chan < numChannels; ++chan )
            Raw<float>()[index + chan] = pixel[chan];
    }
}


// TODO: optimize
RawImage2D RawImage2D::Convert( ImageFormat dstFormat ) const
{
    if ( IsFormatBCCompressed( dstFormat ) )
    {
        LOG_ERR( "RawImage2D::Convert does not support compression, only uncompressed -> uncompressed and compressed -> uncompressed. Please use CompressToBC() instead" );
        return {};
    }

    if ( IsFormatBCCompressed( format ) )
    {
        auto decompressedImg = DecompressBC( *this );
        if ( decompressedImg.format == dstFormat )
        {
            return decompressedImg;
        }
        return decompressedImg.Convert( dstFormat );
    }

    RawImage2D outputImg( width, height, dstFormat );
    uint32_t inputChannels = NumChannels();
    uint32_t outputChannels = outputImg.NumChannels();

    for ( uint32_t row = 0; row < height; ++row )
    {
        for ( uint32_t col = 0; col < width; ++col )
        {
            glm::vec4 pixelAsFloat = GetPixelAsFloat4( row, col );
            outputImg.SetPixelFromFloat4( row, col, pixelAsFloat );
        }
    }

    return outputImg;
}


RawImage2D RawImage2D::Clone() const
{
    RawImage2D ret( width, height, format );
    memcpy( ret.data.get(), data.get(), TotalBytes() );
    return ret;
}


// TODO: load directly
bool FloatImage2D::Load( const std::string& filename )
{
    RawImage2D rawImg;
    if ( !rawImg.Load( filename ) ) return false;

    *this = FloatImageFromRawImage2D( rawImg );
    return true;
}


bool FloatImage2D::Save( const std::string& filename, ImageSaveFlags saveFlags ) const
{
    ImageFormat format = static_cast<ImageFormat>( Underlying( ImageFormat::R32_FLOAT ) + numChannels - 1 );
    RawImage2D img = RawImage2DFromFloatImage( *this, format );
    return img.Save( filename, saveFlags );
}


FloatImage2D FloatImage2D::Resize( uint32_t newWidth, uint32_t newHeight ) const
{
    if ( width == newWidth && height == newHeight )
    {
        return *this;
    }

    FloatImage2D outputImage( newWidth, newHeight, numChannels );
    if ( width == 1 && height == 1 )
    {
        float p[4];
        for ( uint32_t i = 0; i < numChannels; ++i ) p[i] = data[i];

        for ( uint32_t i = 0; i < newWidth * newHeight * numChannels; i += numChannels )
        {
            memcpy( &outputImage.data[i], p, numChannels * sizeof( float ) );
        }
        return outputImage;
    }

    if ( !stbir_resize_float( data.get(), width, height, 0, outputImage.data.get(), newWidth, newHeight, 0, numChannels ) )
    {
        return {};
    }

    return outputImage;
}


FloatImage2D FloatImage2D::Clone() const
{
    FloatImage2D ret( width, height, numChannels );
    memcpy( ret.data.get(), data.get(), width * height * numChannels * sizeof( float ) );
    return ret;
}


glm::vec4 FloatImage2D::Sample( glm::vec2 uv, bool clampHorizontal, bool clampVertical ) const
{
    uv.x -= std::floor( uv.x );
    uv.y -= std::floor( uv.y );

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
        
    glm::vec4 p00 = GetFloat4( row, col, clampHorizontal, clampVertical );
    glm::vec4 p01 = GetFloat4( row, col + 1, clampHorizontal, clampVertical );
    glm::vec4 p10 = GetFloat4( row + 1, col, clampHorizontal, clampVertical );
    glm::vec4 p11 = GetFloat4( row + 1, col + 1, clampHorizontal, clampVertical);
        
    glm::vec4 ret( 0, 0, 0, 1 );
    for ( uint32_t i = 0; i < numChannels; ++i )
    {
        ret[i] = w00 * p00[i] + w01 * p01[i] + w10 * p10[i] + w11 * p11[i];
    }

    return ret;
}


glm::vec4 FloatImage2D::GetFloat4( uint32_t pixelIndex ) const
{
    glm::vec4 pixel( 0, 0, 0, 1 );
    pixelIndex *= numChannels;
    for ( uint32_t chan = 0; chan < numChannels; ++chan )
    {
        pixel[chan] = data[pixelIndex + chan];
    }

    return pixel;
}


glm::vec4 FloatImage2D::GetFloat4( uint32_t row, uint32_t col ) const
{
    return GetFloat4( row * width + col );
}


glm::vec4 FloatImage2D::GetFloat4( uint32_t row, uint32_t col, bool clampHorizontal, bool clampVertical ) const
{
    if ( clampHorizontal )
    {
        col = glm::clamp( col, 0u, width - 1 );
    }
    else
    {
        col = (col % width);
        if ( col < 0 ) col += width;
    }
    if ( clampVertical )
    {
        row = glm::clamp( row, 0u, height - 1 );
    }
    else
    {
        row = (row % height);
        if ( row < 0 ) row += height;
    }

    return GetFloat4( row, col );
}


void FloatImage2D::SetFromFloat4( uint32_t pixelIndex, const glm::vec4& pixel )
{
    pixelIndex *= numChannels;
    for ( uint32_t chan = 0; chan < numChannels; ++chan )
    {
        data[pixelIndex + chan] = pixel[chan];
    }
}


void FloatImage2D::SetFromFloat4( uint32_t row, uint32_t col, const glm::vec4& pixel )
{
    SetFromFloat4( row * width + col, pixel );
}


void HDRImageToLDR( FloatImage2D& image )
{
    image.ForEachPixel( [&]( uint32_t pixelIdx )
    {
        glm::vec4 p = image.GetFloat4( pixelIdx );
        image.SetFromFloat4( pixelIdx, p / (p + glm::vec4( 1.0f )) );
    });
}


// +x -> right, +y -> forward, +z -> up
// uv (0, 0) is upper left corner of image
static glm::vec3 CubemapFaceUVToCartesianDir( int cubeFace, glm::vec2 uv )
{
    uv = 2.0f * uv - glm::vec2( 1.0f );
    glm::vec3 dir;
    switch ( cubeFace )
    {
    case FACE_BACK:
        dir = glm::vec3( -uv.x, -1, -uv.y );
        break;
    case FACE_LEFT:
        dir = glm::vec3( -1, uv.x, -uv.y );
        break;
    case FACE_FRONT:
        dir = glm::vec3( uv.x, 1, -uv.y );
        break;
    case FACE_RIGHT:
        dir = glm::vec3( 1, -uv.x, -uv.y );
        break;
    case FACE_TOP:
        dir = glm::vec3( uv.x, uv.y, 1 );
        break;
    case FACE_BOTTOM:
        dir = glm::vec3( uv.x, -uv.y, -1 );
        break;
    }

    return glm::normalize( dir );
}


// assumes direction is normalized, and right handed +Z up coordinates, upper left face coord = uv (0,0)
static glm::vec2 CubemapDirectionToFaceUV( const glm::vec3& direction, int& faceIndex )
{
    glm::vec3 v = direction;
    glm::vec3 vAbs = abs( v );
	float ma;
	glm::vec2 uv;
	if ( vAbs.y >= vAbs.x && vAbs.y >= vAbs.z )
	{
		faceIndex = v.y < 0.0f ? FACE_BACK : FACE_FRONT;
		ma = 0.5f / vAbs.y;
		uv = glm::vec2( v.y < 0.0f ? -v.x : v.x, -v.z );
	}
	else if ( vAbs.z >= vAbs.x )
	{
		faceIndex = v.z < 0.0f ? FACE_BOTTOM : FACE_TOP;
		ma = 0.5f / vAbs.z;
		uv = glm::vec2( v.x, v.z < 0.0f ? -v.y : v.y );
	}
	else
	{
		faceIndex = v.x < 0.0 ? FACE_LEFT : FACE_RIGHT;
		ma = 0.5f / vAbs.x;
		uv = glm::vec2( v.x < 0.0f ? v.y : -v.y, -v.z );
	}
	return uv * ma + glm::vec2( 0.5f );
}


// http://paulbourke.net/dome/dualfish2sphere/
static glm::vec2 CartesianDirToEquirectangularUV( const glm::vec3& dir )
{
    float phi = atan2( dir.x, dir.y ); // -pi to pi
    float theta = asin( dir.z ); // -pi/2 to pi/2

    float x = phi / PI; // -1 to 1
    float y = 2 * theta / PI; // -1 to 1

    float u = 0.5f * x + 0.5f; // 0 to 1
    float v = 0.5f * -y + 0.5f; // 0 to 1, where 0 = top and 1 = bottom
    return { u, v };
}


bool FloatImageCubemap::LoadFromEquirectangular( const std::string& filename )
{
    FloatImage2D equiImg;
    if ( !equiImg.Load( filename ) )
        return false;

    width = equiImg.width / 4;
    height = width;
    numChannels = equiImg.numChannels;
    for ( uint32_t i = 0; i < 6; ++i )
    {
        faces[i] = FloatImage2D( width, height, numChannels );
        #pragma omp parallel for
        for ( int r = 0; r < (int)height; ++r )
        {
            for ( int c = 0; c < (int)3width; ++ c )
            {
                glm::vec2 localUV = { (c + 0.5f) / (float)width, (r + 0.5f) / (float)height };
                glm::vec3 dir = CubemapFaceUVToCartesianDir( i, localUV );
                glm::vec2 equiUV = CartesianDirToEquirectangularUV( dir );
                faces[i].SetFromFloat4( r, c, equiImg.Sample( equiUV, true, true ) );
            }
        }
    }

    return true;
}


bool FloatImageCubemap::LoadFromFaces( const std::string filenames[6] )
{
    for ( int i = 0; i < 6; ++i )
    {
        if ( !faces[i].Load( filenames[i] ) )
            return false;
    }

    return true;
}


static void CopyImageIntoAnother( FloatImage2D& dstImg, uint32_t dstRow, uint32_t dstCol, const FloatImage2D& srcImg )
{
    PG_ASSERT( dstCol + srcImg.width <= dstImg.width );
    PG_ASSERT( dstRow + srcImg.height <= dstImg.height );
    PG_ASSERT( srcImg.numChannels == dstImg.numChannels );
    for ( uint32_t srcRow = 0; srcRow < srcImg.height; ++srcRow )
    {
        for ( uint32_t srcCol = 0; srcCol < srcImg.width; ++srcCol )
        {
            dstImg.SetFromFloat4( dstRow + srcRow, dstCol + srcCol, srcImg.GetFloat4( srcRow, srcCol ) );
        }
    }
}


bool FloatImageCubemap::SaveUnfoldedFaces( const std::string& filename ) const
{
    if ( !width || !height || !faces[0].data )
    {
        LOG_ERR( "FloatImageCubemap::SaveUnfoldedFaces no image data to save" );
        return false;
    }

    FloatImage2D compositeImg( 4 * width, 3 * height, numChannels );
    CopyImageIntoAnother( compositeImg, height, 0, faces[FACE_LEFT] );
    CopyImageIntoAnother( compositeImg, height, width, faces[FACE_FRONT] );
    CopyImageIntoAnother( compositeImg, height, 2 * width, faces[FACE_RIGHT] );
    CopyImageIntoAnother( compositeImg, height, 3 * width, faces[FACE_BACK] );
    CopyImageIntoAnother( compositeImg, 0, width, faces[FACE_TOP] );
    CopyImageIntoAnother( compositeImg, 2 * height, width, faces[FACE_BOTTOM] );

    return compositeImg.Save( filename );
}


FloatImage2D FloatImageFromRawImage2D( const RawImage2D& rawImage )
{
    FloatImage2D floatImage;
    floatImage.width = rawImage.width;
    floatImage.height = rawImage.height;
    floatImage.numChannels = rawImage.NumChannels();
    if ( IsFormat32BitFloat( rawImage.format ) )
    {
        floatImage.data = std::reinterpret_pointer_cast<float[]>( rawImage.data );
    }
    else
    {
        auto convertedImage = rawImage.Convert( static_cast<ImageFormat>( Underlying( ImageFormat::R32_FLOAT ) + floatImage.numChannels - 1 ) );
        floatImage.data = std::reinterpret_pointer_cast<float[]>( convertedImage.data );
    }

    return floatImage;
}


RawImage2D RawImage2DFromFloatImage( const FloatImage2D& floatImage, ImageFormat format )
{
    RawImage2D rawImage;
    rawImage.width = floatImage.width;
    rawImage.height = floatImage.height;
    rawImage.format = static_cast<ImageFormat>( Underlying( ImageFormat::R32_FLOAT ) + floatImage.numChannels - 1 );
    rawImage.data = std::reinterpret_pointer_cast<uint8_t[]>( floatImage.data );
    if ( format == ImageFormat::INVALID )
    {
        format = rawImage.format;
    }
    if ( rawImage.format != format )
    {
        rawImage = rawImage.Convert( format );
    }

    return rawImage;
}


std::vector<RawImage2D> RawImage2DFromFloatImages( const std::vector<FloatImage2D>& floatImages, ImageFormat format )
{
    std::vector<RawImage2D> rawImages( floatImages.size() );
    for ( size_t i = 0; i < floatImages.size(); ++i )
    {
        rawImages[i] = RawImage2DFromFloatImage( floatImages[i] , format);
    }

    return rawImages;
}


std::vector<FloatImage2D> GenerateMipmaps( const FloatImage2D& image, const MipmapGenerationSettings& settings )
{
    std::vector<FloatImage2D> mips;

    uint32_t numMips = CalculateNumMips( image.width, image.height );
    
    uint32_t w = image.width;
    uint32_t h = image.height;
    uint32_t numChannels = image.numChannels;
    stbir_edge edgeModeU = settings.clampHorizontal ? STBIR_EDGE_CLAMP : STBIR_EDGE_WRAP;
    stbir_edge edgeModeV = settings.clampVertical ? STBIR_EDGE_CLAMP : STBIR_EDGE_WRAP;
    for ( uint32_t mipLevel = 0; mipLevel < numMips; ++mipLevel )
    {
        FloatImage2D mip( w, h, image.numChannels );
        if ( mipLevel == 0 )
        {
            memcpy( mip.data.get(), image.data.get(), w * h * numChannels * sizeof( float ) );
        }
        else
        {
            // NOTE!!! With STBIR_EDGE_WRAP and STBIR_FILTER_MITCHELL, im seeing artifacts, at least for non-even dimensioned images.
            // Both the top and right edges have dark lines near them, and some seemingly garbage pixels 
            const FloatImage2D& prevMip = mips[mipLevel - 1];
            stbir_resize( prevMip.data.get(), prevMip.width, prevMip.height, prevMip.width * numChannels * sizeof( float ),
                          mip.data.get(), w, h, w * numChannels * sizeof( float ), STBIR_TYPE_FLOAT,
                          numChannels, STBIR_ALPHA_CHANNEL_NONE, 0, edgeModeU, edgeModeV, STBIR_FILTER_BOX, STBIR_FILTER_BOX, STBIR_COLORSPACE_LINEAR, NULL );
        }

        mips.push_back( mip );
        w = std::max( 1u, w >> 1 );
        h = std::max( 1u, h >> 1 );
    }

    return mips;
}


uint32_t CalculateNumMips( uint32_t width, uint32_t height )
{
    uint32_t largestDim = std::max( width, height );
    if ( largestDim == 0 )
    {
        return 0;
    }

    return 1 + static_cast< uint32_t >( std::log2f( static_cast< float >( largestDim ) ) );
}


double FloatImageMSE( const FloatImage2D& img1, const FloatImage2D& img2, uint32_t channelsToCalc )
{
    if ( img1.width != img2.width || img1.height != img2.height || img1.numChannels != img2.numChannels )
    {
        LOG_ERR( "Images must be same size and channel count to calculate MSE" );
        return -1;
    }

    uint32_t width = img1.width;
    uint32_t height = img1.height;
    uint32_t numChannels = img1.numChannels;

    double mse = 0;
    for ( uint32_t pixelIdx = 0; pixelIdx < width * height; ++pixelIdx )
    {
        for ( uint32_t chan = 0; chan < numChannels; ++chan )
        {
            if ( channelsToCalc & (1 << (3-chan)) )
            {
                float x = img1.data[numChannels * pixelIdx + chan];
                float y = img2.data[numChannels * pixelIdx + chan];
                mse += (x - y) * (x - y);
            }
        }
    }

    uint32_t numActualChannels = 0;
    for ( uint32_t chan = 0; chan < numChannels; ++chan )
    {
        if ( channelsToCalc & (1 << (3-chan)) )
        {
            ++numActualChannels;
        }
    }

    mse /= (width * height * numActualChannels);
    return mse;
}


double MSEToPSNR( double mse, double maxValue )
{
    return 10.0 * log10( maxValue * maxValue / mse );
}
