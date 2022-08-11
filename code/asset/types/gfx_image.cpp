#include "asset/types/gfx_image.hpp"
#include "glm/glm.hpp"
#include "image.hpp"
#include "renderer/r_globals.hpp"
#include "shared/assert.hpp"
#include "shared/color_spaces.hpp"
#include "shared/float_conversions.hpp"
#include "shared/logger.hpp"
#include "shared/serializer.hpp"
#include "stb/stb_image_resize.h"
#include <algorithm>

namespace PG
{

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
    PG_ASSERT( !PixelFormatIsCompressed( pixelFormat ), "Compression not supported yet" );
    PG_ASSERT( mip < mipLevels );
    PG_ASSERT( pixels );

    int w = width;
    int h = height;
    int bytesPerPixel = NumBytesPerPixel( pixelFormat );
    size_t bytesPerFaceMipChain = CalculateTotalFaceSizeWithMips( width, height, pixelFormat, mipLevels );
    size_t offset = face * bytesPerFaceMipChain;
    for ( uint32_t mipLevel = 0; mipLevel < mip; ++mipLevel )
    {
        offset += w * h * bytesPerPixel;
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

    gpuTexture = r_globals.device.NewTextureFromBuffer( desc, pixels, name );
    PG_ASSERT( gpuTexture );
    free( pixels );
    pixels = nullptr;
#endif // #if USING( GPU )
}


static bool Load_GfxImage_2D( GfxImage* gfxImage, const GfxImageCreateInfo& createInfo )
{
    /*
    gfxImage->width     = w;
    gfxImage->height    = h;
    gfxImage->numFaces  = 1;
    gfxImage->mipLevels = CalculateNumMips( w, h );
    gfxImage->totalSizeInBytes = CalculateTotalFaceSizeWithMips( w, h, gfxImage->pixelFormat );
    gfxImage->pixels = static_cast< unsigned char* >( malloc( gfxImage->totalSizeInBytes ) );

    */
    return false;
}


static bool Load_GfxImage_Cubemap( GfxImage* gfxImage, const GfxImageCreateInfo& createInfo )
{
    /*
    ImageCubemapCreateInfo srcImgCreateInfo;
    if ( createInfo.inputType == ImageInputType::EQUIRECTANGULAR )
    {
        srcImgCreateInfo.equirectangularFilename = createInfo.filename;
    }
    else if ( createInfo.inputType == ImageInputType::FLATTENED_CUBEMAP )
    {
        srcImgCreateInfo.flattenedCubemapFilename = createInfo.filename;
    }
    else if ( createInfo.inputType == ImageInputType::INDIVIDUAL_FACES )
    {
        for ( int i = 0; i < 6; ++i )
        {
            srcImgCreateInfo.faceFilenames[i] = createInfo.faceFilenames[i];
        }
    }
    srcImgCreateInfo.flipVertically = createInfo.flipVertically;
    ImageCubemap srcImg;
    if ( !srcImg.Load( &srcImgCreateInfo ) )
    {
        return false;
    }

    int w = srcImg.faces[0].width;
    int h = srcImg.faces[0].height;
    size_t mipChainSizeInBytes = CalculateTotalFaceSizeWithMips( w, h, PixelFormat::R32_G32_B32_A32_FLOAT );
    glm::vec4* pixelsAllMips = static_cast< glm::vec4* >( malloc( 6 * mipChainSizeInBytes ) );
    int faceOrder[] = { FACE_RIGHT, FACE_LEFT, FACE_TOP, FACE_BOTTOM, FACE_FRONT, FACE_BACK }; // shouldnt the end of this be back then front, not the other way around?
    for ( int face = 0; face < 6; ++face )
    {
        glm::vec4* faceMip0 = pixelsAllMips + face * (mipChainSizeInBytes / sizeof( glm::vec4 ));
        memcpy( faceMip0, srcImg.faces[faceOrder[face]].pixels, w * h * sizeof( glm::vec4 ) );
        GenerateMipmaps_Float32( faceMip0, w, h, faceMip0, createInfo.semantic );
    }

    gfxImage->width     = w;
    gfxImage->height    = h;
    gfxImage->mipLevels = CalculateNumMips( w, h );
    gfxImage->numFaces  = 6;
    gfxImage->totalSizeInBytes = 6 * CalculateTotalFaceSizeWithMips( w, h, gfxImage->pixelFormat );
    gfxImage->pixels = static_cast< unsigned char* >( malloc( gfxImage->totalSizeInBytes ) );
    ConvertRGBA32Float_AllMips( gfxImage->pixels, w, h, gfxImage->numFaces, gfxImage->mipLevels, pixelsAllMips, gfxImage->pixelFormat );
    
    free( pixelsAllMips );


    return true;
    */
    return false;
}


bool GfxImage::Load( const BaseAssetCreateInfo* baseInfo )
{
    return false;
    /*
    PG_ASSERT( baseInfo );
    const GfxImageCreateInfo* createInfo = (const GfxImageCreateInfo*)baseInfo;
    name        = createInfo->name;
    imageType   = createInfo->imageType;
    pixelFormat = createInfo->dstPixelFormat;
    depth       = 1;
    mipLevels   = 1;
    numFaces    = 1;

    if ( createInfo->dstPixelFormat == PixelFormat::INVALID )
    {
        pixelFormat = ChoosePixelFormatFromSemantic( createInfo->semantic );
    }

    bool success;
    if ( createInfo->imageType == Gfx::ImageType::TYPE_2D )
    {
        success = Load_GfxImage_2D( this, *createInfo );
    }
    else if ( createInfo->imageType == Gfx::ImageType::TYPE_CUBEMAP )
    {
        success = Load_GfxImage_Cubemap( this, *createInfo );
    }
    else
    {
        LOG_ERR( "GfxImageType '%d' not supported yet", static_cast< int >( createInfo->imageType ) );
        success = false;
    }
    
    return success;
    */
}


bool GfxImage::FastfileLoad( Serializer* serializer )
{
    static_assert( sizeof( GfxImage ) == sizeof( std::string ) + 56 + sizeof( Gfx::Texture ), "Don't forget to update this function if added/removed members from GfxImage!" );
    
    PG_ASSERT( serializer );
    serializer->Read( name );
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
    static_assert( sizeof( GfxImage ) == sizeof( std::string ) + 56 + sizeof( Gfx::Texture ), "Don't forget to update this function if added/removed members from GfxImage!" );
    
    PG_ASSERT( serializer );
    PG_ASSERT( pixels );
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


GfxImage RawImage2DMipsToGfxImage( const std::vector<RawImage2D>& mips, const std::string& name, bool isSRGB )
{
    if ( mips.empty() ) return {};

    GfxImage gfxImage;
    gfxImage.name = name;
    gfxImage.imageType = Gfx::ImageType::TYPE_2D;
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

} // namespace PG
