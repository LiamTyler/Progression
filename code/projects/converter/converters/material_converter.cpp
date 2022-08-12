#include "base_asset_converter.hpp"
#include "asset/asset_file_database.hpp"
#include "asset/asset_manager.hpp"
#include "asset/types/gfx_image.hpp"
#include "asset/types/material.hpp"
#include "asset/types/textureset.hpp"
#include "core/image_processing.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/hash.hpp"
#include "material_converter.hpp"
#include "shared/hash.hpp"

namespace PG
{

std::string MaterialConverter::GetCacheNameInternal( ConstInfoPtr info )
{
    std::string cacheName = info->name;
    size_t hash = Hash( info->albedoTint );
    HashCombine( hash, info->metalnessTint );
    HashCombine( hash, info->roughnessTint );
    std::string texturesetName = info->texturesetName.empty() ? "default" : info->texturesetName;
    HashCombine( hash, texturesetName );
    auto texsetInfo = AssetDatabase::FindAssetInfo<TexturesetCreateInfo>( ASSET_TYPE_TEXTURESET, texturesetName );
    std::string albedoMetalness = texsetInfo->GetAlbedoMetalnessImageName( info->applyAlbedo, info->applyMetalness );
    //std::string normalRoughness = texsetInfo->GetNormalRoughImageName( info->applyNormals, info->applyRoughness );
    HashCombine( hash, albedoMetalness );
    //HashCombine( hash, normalRoughness );
    cacheName += "_" + std::to_string( hash );
    return cacheName;
}

#define GET_INPUT_TEX_STATUS( timestamp, name ) \
    if ( !name.empty() && !IsImageFilenameBuiltin( name ) ) \
    { \
        AddFastfileDependency( PG_ASSET_DIR + name ); \
        if ( IsFileOutOfDate( timestamp, PG_ASSET_DIR + name ) ) return AssetStatus::OUT_OF_DATE; \
    }

// TODO: Dont mark a material as out of date, unless the image name has actually changed.
// instead these image converts should be added to their a set, for all materials, and then later processed
AssetStatus MaterialConverter::IsAssetOutOfDateInternal( ConstInfoPtr matInfo, time_t materialCacheTimestamp )
{
    std::string texturesetName = matInfo->texturesetName.empty() ? "default" : matInfo->texturesetName;
    auto texsetInfo = AssetDatabase::FindAssetInfo<TexturesetCreateInfo>( ASSET_TYPE_TEXTURESET, texturesetName );
    GET_INPUT_TEX_STATUS( materialCacheTimestamp, texsetInfo->albedoMap );
    GET_INPUT_TEX_STATUS( materialCacheTimestamp, texsetInfo->metalnessMap );
    GET_INPUT_TEX_STATUS( materialCacheTimestamp, texsetInfo->normalMap );
    GET_INPUT_TEX_STATUS( materialCacheTimestamp, texsetInfo->roughnessMap );

    std::string albedoMetalnessCacheName = texsetInfo->GetAlbedoMetalnessImageName( matInfo->applyAlbedo, matInfo->applyMetalness );
    time_t albedoMetalnessTimestamp = AssetCache::GetAssetTimestamp( ASSET_TYPE_GFX_IMAGE, albedoMetalnessCacheName );
    AddFastfileDependency( albedoMetalnessTimestamp );
    if ( IsFileOutOfDate( albedoMetalnessTimestamp, PG_ASSET_DIR + texsetInfo->albedoMap ) ) return AssetStatus::OUT_OF_DATE;
    if ( IsFileOutOfDate( albedoMetalnessTimestamp, PG_ASSET_DIR + texsetInfo->metalnessMap ) ) return AssetStatus::OUT_OF_DATE;

    return AssetStatus::UP_TO_DATE;
}


static bool CreateAndCacheCompositeImage( const CompositeImageInput& compositeInfo, const TexturesetCreateInfo* texsetInfo, const std::string& cacheName )
{
    LOG( "Converting out of date image asset %s...", cacheName.c_str() );
    FloatImage result = CompositeImage( compositeInfo );
    if ( !result.data )
    {
        return false;
    }

    MipmapGenerationSettings settings;
    settings.clampU = texsetInfo->clampU;
    settings.clampV = texsetInfo->clampV;
    std::vector<FloatImage> floatMips = GenerateMipmaps( result, settings );
    std::vector<RawImage2D> compressedMips = RawImage2DFromFloatImages( floatMips, ImageFormat::BC7_UNORM );
    GfxImage gfxImage = RawImage2DMipsToGfxImage( compressedMips, cacheName, true );
    if ( !AssetCache::CacheAsset( ASSET_TYPE_GFX_IMAGE, cacheName, &gfxImage ) )
    {
        LOG_ERR( "Failed to cache composite image asset '%s'", cacheName.c_str() );
        return false;
    }

    return true;
}


static bool FindOrConvertAlbedoMetalnessImage( const MaterialCreateInfo* matInfo, const TexturesetCreateInfo* texsetInfo )
{
    std::string albedoMetalnessName = texsetInfo->GetAlbedoMetalnessImageName( matInfo->applyAlbedo, matInfo->applyMetalness );
    time_t cacheTimestamp = AssetCache::GetAssetTimestamp( ASSET_TYPE_GFX_IMAGE, albedoMetalnessName );
    if ( cacheTimestamp == NO_TIMESTAMP || IsFileOutOfDate( cacheTimestamp, PG_ASSET_DIR + texsetInfo->albedoMap ) || IsFileOutOfDate( cacheTimestamp, PG_ASSET_DIR + texsetInfo->metalnessMap ) )
    {
        CompositeImageInput albedoMetalnessInput;
        albedoMetalnessInput.compositeType = CompositeType::REMAP;
        albedoMetalnessInput.outputColorSpace = ColorSpace::SRGB;
        albedoMetalnessInput.sourceImages.resize( 2 );

        albedoMetalnessInput.sourceImages[0].filename = texsetInfo->albedoMap.empty() ? "$white" : PG_ASSET_DIR + texsetInfo->albedoMap;
        albedoMetalnessInput.sourceImages[0].colorSpace = ColorSpace::SRGB;
        albedoMetalnessInput.sourceImages[0].remaps.push_back( { Channel::R, Channel::R } );
        albedoMetalnessInput.sourceImages[0].remaps.push_back( { Channel::G, Channel::G } );
        albedoMetalnessInput.sourceImages[0].remaps.push_back( { Channel::B, Channel::B } );

        albedoMetalnessInput.sourceImages[1].filename = texsetInfo->metalnessMap.empty() ? "$default_metalness" : PG_ASSET_DIR + texsetInfo->metalnessMap;
        albedoMetalnessInput.sourceImages[1].colorSpace = ColorSpace::LINEAR;
        albedoMetalnessInput.sourceImages[1].remaps.push_back( { Channel::A, Channel::A } );

        return CreateAndCacheCompositeImage( albedoMetalnessInput, texsetInfo, albedoMetalnessName );
    }

    return true;
}


bool MaterialConverter::Convert( const std::string& assetName )
{
    auto matInfo = AssetDatabase::FindAssetInfo<MaterialCreateInfo>( assetType, assetName );
    std::string texturesetName = matInfo->texturesetName.empty() ? "default" : matInfo->texturesetName;
    auto texsetInfo = AssetDatabase::FindAssetInfo<TexturesetCreateInfo>( ASSET_TYPE_TEXTURESET, texturesetName );
    LOG( "Converting out of date asset %s %s...", g_assetNames[assetType], matInfo->name.c_str() );

    if ( !FindOrConvertAlbedoMetalnessImage( matInfo.get(), texsetInfo.get() ) )
    {
        return false;
    }
    
    Material material;
    material.name = matInfo->name;
    material.albedoTint = matInfo->albedoTint;
    material.metalnessTint = matInfo->metalnessTint;
    material.roughnessTint = matInfo->roughnessTint;

    std::string albedoMetalnessName = texsetInfo->GetAlbedoMetalnessImageName( matInfo->applyAlbedo, matInfo->applyMetalness );
    material.albedoMetalnessImage = AssetManager::Get<GfxImage>( albedoMetalnessName );

    std::string cacheName = GetCacheName( matInfo );
    if ( !AssetCache::CacheAsset( assetType, cacheName, &material ) )
    {
        LOG_ERR( "Failed to cache asset %s %s (%s)", g_assetNames[assetType], assetName.c_str(), cacheName.c_str() );
        return false;
    }

    return true;
}


} // namespace PG