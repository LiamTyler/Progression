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

void MaterialConverter::AddGeneratedAssetsInternal( ConstDerivedInfoPtr& matInfo )
{
    std::string texturesetName = matInfo->texturesetName.empty() ? "default" : matInfo->texturesetName;
    auto texturesetInfo = AssetDatabase::FindAssetInfo<TexturesetCreateInfo>( ASSET_TYPE_TEXTURESET, texturesetName );
    std::string albedoMetalnessName = texturesetInfo->GetAlbedoMetalnessImageName( matInfo->applyAlbedo, matInfo->applyMetalness );

    auto imageCreateInfo = std::make_shared<GfxImageCreateInfo>();
    imageCreateInfo->name = albedoMetalnessName;
    imageCreateInfo->semantic = GfxImageSemantic::ALBEDO_METALNESS;
    imageCreateInfo->filenames[0] = texturesetInfo->GetAlbedoMap( matInfo->applyAlbedo );
    imageCreateInfo->filenames[1] = texturesetInfo->GetMetalnessMap( matInfo->applyMetalness );
    imageCreateInfo->clampHorizontal = texturesetInfo->clampHorizontal;
    imageCreateInfo->clampHorizontal = texturesetInfo->clampVertical;
    imageCreateInfo->compositeScales[1] = texturesetInfo->metalnessScale;
    imageCreateInfo->compositeSourceChannels[1] = texturesetInfo->metalnessSourceChannel;

    AddUsedAsset( ASSET_TYPE_GFX_IMAGE, imageCreateInfo );
}

std::string MaterialConverter::GetCacheNameInternal( ConstDerivedInfoPtr info )
{
    std::string cacheName = info->name;
    size_t hash = Hash( info->albedoTint );
    HashCombine( hash, info->metalnessTint );
    //HashCombine( hash, info->roughnessTint );
    std::string texturesetName = info->texturesetName.empty() ? "default" : info->texturesetName;
    HashCombine( hash, texturesetName );
    auto texsetInfo = AssetDatabase::FindAssetInfo<TexturesetCreateInfo>( ASSET_TYPE_TEXTURESET, texturesetName );
    std::string albedoMetalness = texsetInfo->GetAlbedoMetalnessImageName( info->applyAlbedo, info->applyMetalness );
    HashCombine( hash, albedoMetalness );
    //std::string normalRoughness = texsetInfo->GetNormalRoughImageName( info->applyNormals, info->applyRoughness );
    //HashCombine( hash, normalRoughness );
    cacheName += "_" + std::to_string( hash );
    return cacheName;
}


AssetStatus MaterialConverter::IsAssetOutOfDateInternal( ConstDerivedInfoPtr matInfo, time_t materialCacheTimestamp )
{
    // if there is a cache entry at all, it should be up to date (since all fields + image names are part of the material hash name
    return AssetStatus::UP_TO_DATE;
}


bool MaterialConverter::ConvertInternal( ConstDerivedInfoPtr& matInfo )
{
    LOG( "Converting out of date asset %s %s...", g_assetNames[assetType], matInfo->name.c_str() );
    std::string texturesetName = matInfo->texturesetName.empty() ? "default" : matInfo->texturesetName;
    auto texturesetInfo = AssetDatabase::FindAssetInfo<TexturesetCreateInfo>( ASSET_TYPE_TEXTURESET, texturesetName );

    Material material;
    material.name = matInfo->name;
    material.albedoTint = matInfo->albedoTint;
    material.metalnessTint = matInfo->metalnessTint;
    material.albedoMetalnessImage = AssetManager::Get<GfxImage>( texturesetInfo->GetAlbedoMetalnessImageName( matInfo->applyAlbedo, matInfo->applyMetalness ) );
    //material.roughnessTint = matInfo->roughnessTint;

    std::string cacheName = GetCacheName( matInfo );
    if ( !AssetCache::CacheAsset( assetType, cacheName, &material ) )
    {
        LOG_ERR( "Failed to cache asset %s %s (%s)", g_assetNames[assetType], matInfo->name.c_str(), cacheName.c_str() );
        return false;
    }

    return true;
}


} // namespace PG