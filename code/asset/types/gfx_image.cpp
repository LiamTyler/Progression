#include "asset/types/gfx_image.hpp"
#include "core/image_processing.hpp"
#include "ImageLib/bc_compression.hpp"
#include "shared/assert.hpp"
#include "shared/logger.hpp"
#include "shared/serializer.hpp"
#include <algorithm>

#if USING( GPU_DATA )
#include "renderer/r_globals.hpp"
#endif // #if USING( GPU_DATA )

static constexpr CompressionQuality COMPRESSOR_QUALITY = CompressionQuality::MEDIUM;

namespace PG
{

bool IsSemanticComposite( GfxImageSemantic semantic )
{
    static_assert( Underlying( GfxImageSemantic::NUM_IMAGE_SEMANTICS ) == 5 );
    return semantic == GfxImageSemantic::ALBEDO_METALNESS;
}


void GfxImage::Free()
{
    if ( pixels )
    {
        free( pixels );
        pixels = nullptr;
    }
#if USING( GPU_DATA )
    gpuTexture.Free();
#endif // #if USING( GPU )
}


unsigned char* GfxImage::GetPixels( uint32_t face, uint32_t mip, uint32_t depthLevel ) const
{
    PG_ASSERT( depthLevel == 0, "Texture arrays not supported yet" );
    PG_ASSERT( mip < mipLevels );
    PG_ASSERT( pixels );

    int w = width;
    int h = height;
    int bytesPerPixel = NumBytesPerPixel( pixelFormat );
    bool isCompressed = PixelFormatIsCompressed( pixelFormat );
    size_t bytesPerFaceMipChain = CalculateTotalFaceSizeWithMips( width, height, pixelFormat, mipLevels );
    size_t offset = face * bytesPerFaceMipChain;
    for ( uint32_t mipLevel = 0; mipLevel < mip; ++mipLevel )
    {
        uint32_t paddedWidth  = isCompressed ? (w + 3) & ~3u : w;
        uint32_t paddedHeight = isCompressed ? (h + 3) & ~3u : h;
        uint32_t size = paddedWidth * paddedHeight * bytesPerPixel;
        if ( isCompressed ) size /= 16;
        offset += size;
        w = std::max( 1, w >> 1 );
        h = std::max( 1, h >> 1 );
    }

    return pixels + offset;
}


void GfxImage::UploadToGpu()
{
#if USING( GPU_DATA )
    using namespace Gfx;
    PG_ASSERT( pixels );
    
    if ( gpuTexture )
    {
        gpuTexture.Free();
    }

    TextureDescriptor desc;
    desc.format      = pixelFormat;
    desc.type        = imageType;
    desc.width       = width;
    desc.height      = height;
    desc.depth       = depth;
    desc.arrayLayers = numFaces;
    desc.mipLevels   = mipLevels;
    desc.usage       = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    desc.addToBindlessArray = imageType == ImageType::TYPE_2D;

    gpuTexture = rg.device.NewTextureFromBuffer( desc, pixels, name );
    PG_ASSERT( gpuTexture );
    free( pixels );
    pixels = nullptr;
#endif // #if USING( GPU )
}


static std::string GetImageFullPath( const std::string& filename )
{
    return IsImageFilenameBuiltin( filename ) ? filename : PG_ASSET_DIR + filename;
}


static bool Load_AlbedoMetalness( GfxImage* gfxImage, const GfxImageCreateInfo* createInfo )
{
    CompositeImageInput compositeInfo;
    compositeInfo.compositeType = CompositeType::REMAP;
    compositeInfo.outputColorSpace = ColorSpace::SRGB;
    compositeInfo.sourceImages.resize( 2 );

    compositeInfo.sourceImages[0].filename = GetImageFullPath( createInfo->filenames[0] );
    compositeInfo.sourceImages[0].colorSpace = ColorSpace::SRGB;
    compositeInfo.sourceImages[0].remaps.push_back( { Channel::R, Channel::R } );
    compositeInfo.sourceImages[0].remaps.push_back( { Channel::G, Channel::G } );
    compositeInfo.sourceImages[0].remaps.push_back( { Channel::B, Channel::B } );

    compositeInfo.sourceImages[1].filename = GetImageFullPath( createInfo->filenames[1] );
    compositeInfo.sourceImages[1].colorSpace = ColorSpace::LINEAR;
    compositeInfo.sourceImages[1].remaps.push_back( { createInfo->compositeSourceChannels[1], Channel::A } );

    FloatImage composite = CompositeImage( compositeInfo );
    if ( !composite.data )
    {
        return false;
    }

    MipmapGenerationSettings settings;
    settings.clampHorizontal = createInfo->clampHorizontal;
    settings.clampVertical = createInfo->clampVertical;
    std::vector<RawImage2D> rawMipsFloat32 = RawImage2DFromFloatImages( GenerateMipmaps( composite, settings ) );
    
    BCCompressorSettings compressorSettings( ImageFormat::BC7_UNORM, COMPRESSOR_QUALITY );
    std::vector<RawImage2D> compressedMips = CompressToBC( rawMipsFloat32, compressorSettings );

    *gfxImage = RawImage2DMipsToGfxImage( compressedMips, compositeInfo.outputColorSpace == ColorSpace::SRGB );

    return gfxImage->pixels != nullptr;
}


static bool Load_EnvironmentMap_EquiRectangular( GfxImage* gfxImage, const GfxImageCreateInfo* createInfo, RawImage2D &img )
{
    BCCompressorSettings compressorSettings( ImageFormat::BC6H_U16F, COMPRESSOR_QUALITY );
    RawImage2D compressed = CompressToBC( img, compressorSettings );

    *gfxImage = RawImage2DMipsToGfxImage( { compressed }, false );

    return gfxImage->pixels != nullptr;
}


static bool Load_UI( GfxImage* gfxImage, const GfxImageCreateInfo* createInfo )
{
    RawImage2D img;
    if ( !img.Load( GetImageFullPath( createInfo->filenames[0] ) ) )
        return false;

    BCCompressorSettings compressorSettings( ImageFormat::BC7_UNORM, COMPRESSOR_QUALITY );
    RawImage2D compressed = CompressToBC( img, compressorSettings );

    *gfxImage = RawImage2DMipsToGfxImage( { compressed }, false );

    return gfxImage->pixels != nullptr;
}


static bool Load_EnvironmentMap_CubeMap( GfxImage* gfxImage, const GfxImageCreateInfo* createInfo, RawImage2D (&faces)[6] )
{
    return false; // TODO
}


static bool Load_EnvironmentMap( GfxImage* gfxImage, const GfxImageCreateInfo* createInfo )
{
    RawImage2D faces[6];
    int numFaces = 0;
    for ( int i = 0; i < 6; ++i )
    {
        if ( !createInfo->filenames[i].empty() )
        {
            if ( !faces[i].Load( GetImageFullPath( createInfo->filenames[i] ) ) )
            {
                return false;
            }
            ++numFaces;
        }
    }
    
    if ( numFaces == 1 )
    {
        PG_ASSERT( !createInfo->filenames[0].empty(), "Filename must be in first slot, for non-cubemap images" );
        return Load_EnvironmentMap_EquiRectangular( gfxImage, createInfo, faces[0] );
    }
    else if ( numFaces == 6 )
    {
        return Load_EnvironmentMap_CubeMap( gfxImage, createInfo, faces );
    }
    else
    {
        LOG_ERR( "Unrecognized number of faces for environment map: %d. Only 1 (equirectangular) or 6 (cubemap) supported", numFaces );
        return false;
    }
}


bool GfxImage::Load( const BaseAssetCreateInfo* baseInfo )
{
    PG_ASSERT( baseInfo );
    const GfxImageCreateInfo* createInfo = (const GfxImageCreateInfo*)baseInfo;

    bool success = false;
    switch ( createInfo->semantic )
    {
    case GfxImageSemantic::ALBEDO_METALNESS:
        success = Load_AlbedoMetalness( this, createInfo );
        break;
    case GfxImageSemantic::ENVIRONMENT_MAP:
        success = Load_EnvironmentMap( this, createInfo );
        break;
    case GfxImageSemantic::UI:
        success = Load_UI( this, createInfo );
        break;
    default:
        LOG_ERR( "GfxImage::Load not implemented yet for semantic %u", Underlying( createInfo->semantic ) );
        return false;
    }
    name = createInfo->name;
    
    return success;
}


bool GfxImage::FastfileLoad( Serializer* serializer )
{
    PG_ASSERT( serializer );
    serializer->Read( name );
    PG_ASSERT( name != "" );
    serializer->Read( width );
    serializer->Read( height );
    serializer->Read( depth );
    serializer->Read( mipLevels );
    serializer->Read( numFaces );
    serializer->Read( totalSizeInBytes );
    serializer->Read( pixelFormat );
    serializer->Read( imageType );
    pixels = static_cast< unsigned char* >( malloc( totalSizeInBytes ) );
    serializer->Read( pixels, totalSizeInBytes );

    UploadToGpu();

    return true;
}


bool GfxImage::FastfileSave( Serializer* serializer ) const
{
    PG_ASSERT( serializer );
    PG_ASSERT( pixels );
    PG_ASSERT( name != "" );
    serializer->Write( name );
    serializer->Write( width );
    serializer->Write( height );
    serializer->Write( depth );
    serializer->Write( mipLevels );
    serializer->Write( numFaces );
    serializer->Write( totalSizeInBytes );
    serializer->Write( pixelFormat );
    serializer->Write( imageType );
    serializer->Write( pixels, totalSizeInBytes );

    return true;
}


size_t CalculateTotalFaceSizeWithMips( uint32_t width, uint32_t height, PixelFormat format, uint32_t numMips )
{
    PG_ASSERT( width > 0 && height > 0 );
    PG_ASSERT( format != PixelFormat::INVALID );
    bool isCompressed = PixelFormatIsCompressed( format );
    uint32_t w = width;
    uint32_t h = height;
    if ( numMips == 0 )
    {
        numMips = CalculateNumMips( w, h );
    }

    uint32_t bytesPerPixel = NumBytesPerPixel( format );
    size_t currentSize = 0;
    for ( uint32_t mipLevel = 0; mipLevel < numMips; ++mipLevel )
    {
        uint32_t paddedWidth  = isCompressed ? (w + 3) & ~3u : w;
        uint32_t paddedHeight = isCompressed ? (h + 3) & ~3u : h;
        uint32_t size = paddedWidth * paddedHeight * bytesPerPixel;
        if ( isCompressed ) size /= 16;
        currentSize += size;

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


PixelFormat ImageFormatToPixelFormat( ImageFormat imgFormat, bool isSRGB )
{
    static_assert( Underlying( ImageFormat::COUNT ) == 27, "don't forget to update this switch statement" );
    switch ( imgFormat )
    {
    case ImageFormat::R8_UNORM:
        return isSRGB ? PixelFormat::R8_SRGB : PixelFormat::R8_UNORM;
    case ImageFormat::R8_G8_UNORM:
        return isSRGB ? PixelFormat::R8_G8_SRGB : PixelFormat::R8_G8_UNORM;
    case ImageFormat::R8_G8_B8_UNORM:
        return isSRGB ? PixelFormat::R8_G8_B8_SRGB : PixelFormat::R8_G8_B8_UNORM;
    case ImageFormat::R8_G8_B8_A8_UNORM:
        return isSRGB ? PixelFormat::R8_G8_B8_A8_SRGB : PixelFormat::R8_G8_B8_A8_UNORM;

    case ImageFormat::R16_UNORM:
        return PixelFormat::R16_UNORM;
    case ImageFormat::R16_G16_UNORM:
        return PixelFormat::R16_G16_UNORM;
    case ImageFormat::R16_G16_B16_UNORM:
        return PixelFormat::R16_G16_B16_UNORM;
    case ImageFormat::R16_G16_B16_A16_UNORM:
        return PixelFormat::R16_G16_B16_A16_UNORM;

    case ImageFormat::R16_FLOAT:
        return PixelFormat::R16_FLOAT;
    case ImageFormat::R16_G16_FLOAT:
        return PixelFormat::R16_G16_FLOAT;
    case ImageFormat::R16_G16_B16_FLOAT:
        return PixelFormat::R16_G16_B16_FLOAT;
    case ImageFormat::R16_G16_B16_A16_FLOAT:
        return PixelFormat::R16_G16_B16_A16_FLOAT;

    case ImageFormat::R32_FLOAT:
        return PixelFormat::R32_FLOAT;
    case ImageFormat::R32_G32_FLOAT:
        return PixelFormat::R32_G32_FLOAT;
    case ImageFormat::R32_G32_B32_FLOAT:
        return PixelFormat::R32_G32_B32_FLOAT;
    case ImageFormat::R32_G32_B32_A32_FLOAT:
        return PixelFormat::R32_G32_B32_A32_FLOAT;

    case ImageFormat::BC1_UNORM:
        return isSRGB ? PixelFormat::BC1_RGB_SRGB : PixelFormat::BC1_RGB_UNORM;
    case ImageFormat::BC2_UNORM:
        return isSRGB ? PixelFormat::BC2_SRGB : PixelFormat::BC2_UNORM;
    case ImageFormat::BC3_UNORM:
        return isSRGB ? PixelFormat::BC3_SRGB : PixelFormat::BC3_UNORM;
    case ImageFormat::BC4_UNORM:
        return PixelFormat::BC4_UNORM;
    case ImageFormat::BC4_SNORM:
        return PixelFormat::BC4_SNORM;
    case ImageFormat::BC5_UNORM:
        return PixelFormat::BC5_UNORM;
    case ImageFormat::BC5_SNORM:
        return PixelFormat::BC5_SNORM;
    case ImageFormat::BC6H_U16F:
        return PixelFormat::BC6H_UFLOAT;
    case ImageFormat::BC6H_S16F:
        return PixelFormat::BC6H_SFLOAT;
    case ImageFormat::BC7_UNORM:
        return isSRGB ? PixelFormat::BC7_SRGB : PixelFormat::BC7_UNORM;

    default:
        LOG_WARN( "ImageFormatToPixelFormat: Image format %u not recognized", Underlying( imgFormat ) );
        return PixelFormat::INVALID;
    }
}


GfxImage RawImage2DMipsToGfxImage( const std::vector<RawImage2D>& mips, bool isSRGB )
{
    if ( mips.empty() ) return {};

    GfxImage gfxImage;
    gfxImage.imageType = ImageType::TYPE_2D;
    gfxImage.width = mips[0].width;
    gfxImage.height = mips[0].height;
    gfxImage.depth = 1;
    gfxImage.numFaces = 1;
    gfxImage.mipLevels = static_cast<uint32_t>( mips.size() );
    gfxImage.pixelFormat = ImageFormatToPixelFormat( mips[0].format, isSRGB );
    gfxImage.totalSizeInBytes = CalculateTotalImageBytes( gfxImage.pixelFormat, gfxImage.width, gfxImage.height, 1, 1, gfxImage.mipLevels );
    gfxImage.pixels = static_cast<uint8_t*>( malloc( gfxImage.totalSizeInBytes ) );

    uint8_t* currentMip = gfxImage.pixels;
    for ( uint32_t mipLevel = 0; mipLevel < gfxImage.mipLevels; ++mipLevel )
    {
        const RawImage2D& mip = mips[mipLevel];
        memcpy( currentMip, mip.Raw(), mip.TotalBytes() );
        currentMip += mip.TotalBytes();
    }

    return gfxImage;
}


GfxImage DecompressGfxImage( const GfxImage &compressedImage )
{
    if ( !PixelFormatIsCompressed( compressedImage.pixelFormat ) )
    {
        return compressedImage;
    }

    PG_ASSERT( compressedImage.depth == 1, "TODO: support depth > 1" );
    ImageFormat compressedImgFormat = PixelFormatToImageFormat( compressedImage.pixelFormat );
    ImageFormat decompressedImgFormat = GetFormatAfterDecompression( compressedImgFormat );
    PixelFormat decompressedPixelFormat = ImageFormatToPixelFormat( decompressedImgFormat, PixelFormatIsSrgb( compressedImage.pixelFormat ) );

    GfxImage decompressedImg = compressedImage;
    decompressedImg.pixelFormat = decompressedPixelFormat;
    decompressedImg.totalSizeInBytes = CalculateTotalImageBytes( decompressedPixelFormat, compressedImage.width, compressedImage.height, compressedImage.depth, compressedImage.numFaces, compressedImage.mipLevels );
    decompressedImg.pixels = static_cast<uint8_t*>( malloc( decompressedImg.totalSizeInBytes ) );

    for ( uint32_t face = 0; face < compressedImage.numFaces; ++face )
    {
        uint32_t w = compressedImage.width;
        uint32_t h = compressedImage.height;
        for ( uint32_t mipLevel = 0; mipLevel < compressedImage.mipLevels; ++mipLevel )
        {
            RawImage2D compressedMip( w, h, compressedImgFormat, compressedImage.GetPixels( face, mipLevel ) );
            RawImage2D decompressedMip = DecompressBC( compressedMip );
            memcpy( decompressedImg.GetPixels( face, mipLevel ), decompressedMip.Raw(), decompressedMip.TotalBytes() );

            w = std::max( 1u, w >> 1 );
            h = std::max( 1u, h >> 1 );
        }
    }
    
    return decompressedImg;
}

} // namespace PG
