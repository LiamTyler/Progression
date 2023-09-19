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
    static_assert( Underlying( GfxImageSemantic::NUM_IMAGE_SEMANTICS ) == 7 );
    return semantic == GfxImageSemantic::ALBEDO_METALNESS || semantic == GfxImageSemantic::NORMAL_ROUGHNESS;
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
    desc.addToBindlessArray = imageType == ImageType::TYPE_2D || imageType == ImageType::TYPE_CUBEMAP;

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
    compositeInfo.flipVertically = createInfo->flipVertically;
    compositeInfo.sourceImages.resize( 2 );

    compositeInfo.sourceImages[0].filename = GetImageFullPath( createInfo->filenames[0] );
    compositeInfo.sourceImages[0].colorSpace = ColorSpace::SRGB;
    compositeInfo.sourceImages[0].remaps.push_back( { Channel::R, Channel::R } );
    compositeInfo.sourceImages[0].remaps.push_back( { Channel::G, Channel::G } );
    compositeInfo.sourceImages[0].remaps.push_back( { Channel::B, Channel::B } );

    compositeInfo.sourceImages[1].filename = GetImageFullPath( createInfo->filenames[1] );
    compositeInfo.sourceImages[1].colorSpace = ColorSpace::LINEAR;
    compositeInfo.sourceImages[1].remaps.push_back( { createInfo->compositeSourceChannels[1], Channel::A } );

    FloatImage2D composite = CompositeImage( compositeInfo );
    if ( !composite.data )
    {
        return false;
    }

    composite.ForEachPixel( [createInfo]( float* p )
    {
        p[3] *= createInfo->compositeScales[1]; // metalnessScale
    });

    MipmapGenerationSettings settings;
    settings.clampHorizontal = createInfo->clampHorizontal;
    settings.clampVertical = createInfo->clampVertical;
    std::vector<RawImage2D> rawMipsFloat32 = RawImage2DFromFloatImages( GenerateMipmaps( composite, settings ) );
    
    BCCompressorSettings compressorSettings( ImageFormat::BC7_UNORM, COMPRESSOR_QUALITY );
    std::vector<RawImage2D> compressedMips = CompressToBC( rawMipsFloat32, compressorSettings );

    *gfxImage = RawImage2DMipsToGfxImage( compressedMips, compositeInfo.outputColorSpace == ColorSpace::SRGB );

    return gfxImage->pixels != nullptr;
}


static glm::vec3 ScaleNormal( glm::vec3 n, float scale )
{
    n.x *= scale;
    n.y *= scale;

    return normalize( n );
}


static bool Load_NormalRoughness( GfxImage* gfxImage, const GfxImageCreateInfo* createInfo )
{
    CompositeImageInput compositeInfo;
    compositeInfo.compositeType = CompositeType::REMAP;
    compositeInfo.outputColorSpace = ColorSpace::LINEAR;
    compositeInfo.flipVertically = createInfo->flipVertically;
    compositeInfo.sourceImages.resize( 2 );

    compositeInfo.sourceImages[0].filename = GetImageFullPath( createInfo->filenames[0] );
    compositeInfo.sourceImages[0].colorSpace = ColorSpace::LINEAR;
    compositeInfo.sourceImages[0].remaps.push_back( { Channel::R, Channel::R } );
    compositeInfo.sourceImages[0].remaps.push_back( { Channel::G, Channel::G } );
    compositeInfo.sourceImages[0].remaps.push_back( { Channel::B, Channel::B } );

    compositeInfo.sourceImages[1].filename = GetImageFullPath( createInfo->filenames[1] );
    compositeInfo.sourceImages[1].colorSpace = ColorSpace::LINEAR;
    compositeInfo.sourceImages[1].remaps.push_back( { createInfo->compositeSourceChannels[1], Channel::A } );

    FloatImage2D composite = CompositeImage( compositeInfo );
    if ( !composite.data )
    {
        return false;
    }

    float roughnessScale = createInfo->compositeScales[2];
    bool invertRoughness = createInfo->compositeScales[3];
    if ( roughnessScale != 1.0f || invertRoughness )
    {
        float roughnessBias = invertRoughness ? 1.0f : 0;
        if ( invertRoughness )
            roughnessScale *= -1;

        composite.ForEachPixel( [roughnessBias, roughnessScale]( float* p )
        {
            p[3] = roughnessBias + roughnessScale * p[3];
        });
    }

    float slopeScale = createInfo->compositeScales[0];
    bool normalMapIsYUp = createInfo->compositeScales[1];
    if ( slopeScale != 1.0f || !normalMapIsYUp )
    {
        composite.ForEachPixel( [slopeScale, normalMapIsYUp]( float* p )
        {
            glm::vec3 normal = { p[0], p[1], p[2] };
            if ( !normalMapIsYUp )
                normal.y *= -1;
            normal = ScaleNormal( normal, slopeScale );
            p[0] = normal.x; p[1] = normal.y; p[2] = normal.z;
        });
    }

    MipmapGenerationSettings settings;
    settings.clampHorizontal = createInfo->clampHorizontal;
    settings.clampVertical = createInfo->clampVertical;
    // TODO: alter the normals based on the roughness when mipmapping, because the macro-level normals
    // effectively become micro-level detail as you downsample
    std::vector<FloatImage2D> floatMips = GenerateMipmaps( composite, settings );
    for ( uint32_t mipLevel = 1; mipLevel < (uint32_t)floatMips.size(); ++mipLevel )
    {
        floatMips[mipLevel].ForEachPixel( []( float* p )
        {
            glm::vec3 normal = { p[0], p[1], p[2] };
            normal = normalize( normal );
            p[0] = normal.x; p[1] = normal.y; p[2] = normal.z;
        });
    }
    std::vector<RawImage2D> rawMipsFloat32 = RawImage2DFromFloatImages( floatMips );
    
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
    ImageLoadFlags imgLoadFlags = createInfo->flipVertically ? ImageLoadFlags::FLIP_VERTICALLY : ImageLoadFlags::DEFAULT;
    RawImage2D img;
    if ( !img.Load( GetImageFullPath( createInfo->filenames[0] ), imgLoadFlags ) )
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


static void FlipColumns( FloatImage2D& image )
{
    for ( uint32_t row = 0; row < image.height; ++row )
    {
        for ( uint32_t col = 0; col < image.width / 2; ++col )
        {
            uint32_t endCol = image.width - col - 1;
            auto p0 = image.GetFloat4( row, col );
            auto p1 = image.GetFloat4( row, endCol );
            image.SetFromFloat4( row, col, p1 );
            image.SetFromFloat4( row, endCol, p0 );
        }
    }
}

static void FlipRows( FloatImage2D& image )
{
    for ( uint32_t row = 0; row < image.height / 2; ++row )
    {
        for ( uint32_t col = 0; col < image.width; ++col )
        {
            uint32_t endRow = image.height - row - 1;
            auto p0 = image.GetFloat4( row, col );
            auto p1 = image.GetFloat4( endRow, col );
            image.SetFromFloat4( row, col, p1 );
            image.SetFromFloat4( endRow, col, p0 );
        }
    }
}

static void FlipMainDiag( FloatImage2D& image )
{
    PG_ASSERT( image.width == image.height );
    for ( uint32_t row = 0; row < image.height; ++row )
    {
        for ( uint32_t col = row + 1; col < image.width; ++col )
        {
            auto p0 = image.GetFloat4( row, col );
            auto p1 = image.GetFloat4( col, row );
            image.SetFromFloat4( row, col, p1 );
            image.SetFromFloat4( col, row, p0 );
        }
    }
}

static void FlipReverseDiag( FloatImage2D& image )
{
    PG_ASSERT( image.width == image.height );
    for ( uint32_t row = 0; row < image.height; ++row )
    {
        for ( uint32_t col = 0; col < image.height - row - 1; ++col )
        {
            uint32_t tRow = image.height - col - 1;
            uint32_t tCol = image.width - row - 1;
            auto p0 = image.GetFloat4( row, col );
            auto p1 = image.GetFloat4( tRow, tCol );
            image.SetFromFloat4( row, col, p1 );
            image.SetFromFloat4( tRow, tCol, p0 );
        }
    }
}

static void ConvertPGCubemapToVkCubemap( FloatImageCubemap& cubemap )
{
    // https://registry.khronos.org/vulkan/specs/1.3/html/chap16.html#_cube_map_face_selection
    // Vulkan cubemap sampling expects left-handed(?) Y-up in when deciding the per-face UVs. Need to
    // rotate/flip the pixels in each face accordingly to account for it
    FlipColumns( cubemap.faces[0] );
    FlipReverseDiag( cubemap.faces[1] );
    FlipRows( cubemap.faces[2] );
    FlipMainDiag( cubemap.faces[3] );
    FlipRows( cubemap.faces[4] );
    FlipColumns( cubemap.faces[5] );

    // From https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImageSubresourceRange.html
    // the layers of the image view starting at baseArrayLayer correspond to faces in the order +X, -X, +Y, -Y, +Z, -Z
    FaceIndex pgToVkFaceOrder[6] = { FACE_RIGHT, FACE_LEFT, FACE_FRONT, FACE_BACK, FACE_TOP, FACE_BOTTOM };
    FloatImage2D reorderedFaces[6];
    for ( int i = 0; i < 6; ++i )
    {
        reorderedFaces[i] = cubemap.faces[pgToVkFaceOrder[i]];
    }
    for ( int i = 0; i < 6; ++i )
    {
        cubemap.faces[i] = reorderedFaces[i];
    }
}


static FloatImageCubemap CreateDebugCubemap( uint32_t size )
{
    const glm::vec3 faceColors[6] = 
    {
        glm::vec3( 0, 0, 1 ), // FACE_BACK
        glm::vec3( 0, 1, 0 ), // FACE_LEFT
        glm::vec3( 1, 0, 0 ), // FACE_FRONT
        glm::vec3( 1, 1, 0 ), // FACE_RIGHT
        glm::vec3( 0, 1, 1 ), // FACE_TOP
        glm::vec3( 1, 0, 1 ), // FACE_BOTTOM
    };

    FloatImageCubemap cubemap( 32, 3 );
    for ( int faceIdx = 0; faceIdx < 6; ++faceIdx )
    {
        for ( int dstRow = 0; dstRow < (int)cubemap.size; ++dstRow )
        {
            for ( int dstCol = 0; dstCol < (int)cubemap.size; ++dstCol )
            {
                glm::vec2 faceUV = { (dstCol + 0.5f) / cubemap.size, (dstRow + 0.5f) / cubemap.size };
                glm::vec3 vert = faceColors[faceIdx];
                glm::vec3 hori = glm::vec3( 1 );
                glm::vec3 b = faceUV.x * hori + faceUV.y * vert;
                b /= std::max( 1.0f, (faceUV.x + faceUV.y) );
                cubemap.faces[faceIdx].SetFromFloat4( dstRow, dstCol, glm::vec4( b, 1 ) );
            }
        }
    }

    return cubemap;
}


static bool Load_EnvironmentMapIrradiance( GfxImage* gfxImage, const GfxImageCreateInfo* createInfo )
{
    int numFaces = 0;
    std::string filenames[6];
    for ( int i = 0; i < 6; ++i )
    {
        if ( !createInfo->filenames[i].empty() )
        {
            filenames[i] = GetImageFullPath( createInfo->filenames[i] );
            ++numFaces;
        }
    }
    
    FloatImageCubemap cubemap;
    if ( numFaces == 1 )
    {
        PG_ASSERT( !filenames[0].empty(), "Filename must be in first slot, for equirectangular inputs" );
        if ( !cubemap.LoadFromEquirectangular( filenames[0] ) )
            return false;
    }
    else if ( numFaces == 6 )
    {
        if ( !cubemap.LoadFromFaces( filenames ) )
            return false;
    }
    else
    {
        LOG_ERR( "Unrecognized number of faces for environment map: %d. Only 1 (equirectangular) or 6 (cubemap) supported", numFaces );
        return false;
    }

    FloatImageCubemap irradianceMap( 32, cubemap.numChannels );
    for ( int faceIdx = 0; faceIdx < 6; ++faceIdx )
    {
        #pragma omp parallel for
        for ( int dstRow = 0; dstRow < (int)irradianceMap.size; ++dstRow )
        {
            for ( int dstCol = 0; dstCol < (int)irradianceMap.size; ++dstCol )
            {
                glm::vec2 faceUV = { (dstCol + 0.5f) / irradianceMap.size, (dstRow + 0.5f) / irradianceMap.size };
                
                glm::vec3 normal = CubemapFaceUVToDirection( faceIdx, faceUV );
                glm::vec3 tangent = normal.x < 0.99f ? glm::vec3( 1, 0, 0 ) : glm::vec3( 0, 1, 0 );
                glm::vec3 bitangent = glm::normalize( glm::cross( normal, tangent ) );
                tangent = glm::normalize( glm::cross( bitangent, normal ) );

                constexpr float angleDelta = PI / 60;
                uint32_t numSamples = 0;
                glm::vec3 irradiance( 0 );
                for ( float phi = 0; phi < 2 * PI; phi += angleDelta )
                {
                    for ( float theta = 0; theta < PI / 2; theta += angleDelta )
                    {
                        glm::vec3 tangentSpaceDir = { sin( theta ) * cos( phi ), sin( theta ) * sin( phi ), cos( theta ) };
                        glm::vec3 worldSpaceDir = tangentSpaceDir.x * tangent + tangentSpaceDir.y * bitangent + tangentSpaceDir.z * normal;
                        glm::vec3 radiance = cubemap.Sample( worldSpaceDir );
                        irradiance += radiance * cos( theta ) * sin( theta );
                        ++numSamples;
                    }
                }

                irradiance *= (PI / numSamples);
                irradianceMap.faces[faceIdx].SetFromFloat4( dstRow, dstCol, glm::vec4( irradiance, 0 ) );
            }
        }
        LOG( "Convolved face %d / 6...", faceIdx + 1 );
    }

    ConvertPGCubemapToVkCubemap( irradianceMap );

    RawImage2D faces[6];
    BCCompressorSettings compressorSettings( ImageFormat::BC6H_U16F, COMPRESSOR_QUALITY );
    for ( int i = 0; i < 6; ++i )
    {
        RawImage2D face = RawImage2DFromFloatImage( irradianceMap.faces[i], ImageFormat::R16_G16_B16_FLOAT );
        faces[i] = CompressToBC( face, compressorSettings );
    }

    gfxImage->imageType = ImageType::TYPE_2D;
    gfxImage->width = irradianceMap.size;
    gfxImage->height = irradianceMap.size;
    gfxImage->depth = 1;
    gfxImage->numFaces = 6;
    gfxImage->mipLevels = 1;
    gfxImage->pixelFormat = ImageFormatToPixelFormat( ImageFormat::BC6H_U16F, false );
    gfxImage->totalSizeInBytes = CalculateTotalImageBytes( *gfxImage );
    gfxImage->pixels = static_cast<uint8_t*>( malloc( gfxImage->totalSizeInBytes ) );

    uint8_t* currentFace = gfxImage->pixels;
    for ( int i = 0; i < 6; ++i )
    {
        memcpy( currentFace, faces[i].Raw(), faces[i].TotalBytes() );
        currentFace += faces[i].TotalBytes();
    }

    return true;
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
    case GfxImageSemantic::NORMAL_ROUGHNESS:
        success = Load_NormalRoughness( this, createInfo );
        break;
    case GfxImageSemantic::ENVIRONMENT_MAP:
        success = Load_EnvironmentMap( this, createInfo );
        break;
    case GfxImageSemantic::ENVIRONMENT_MAP_IRRADIANCE:
        success = Load_EnvironmentMapIrradiance( this, createInfo );
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


size_t CalculateTotalImageBytes( const GfxImage& img )
{
    return CalculateTotalImageBytes( img.pixelFormat, img.width, img.height, img.depth, img.numFaces, img.mipLevels );
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
