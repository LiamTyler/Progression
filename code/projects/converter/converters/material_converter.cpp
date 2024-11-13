#include "material_converter.hpp"
#include "asset/asset_file_database.hpp"
#include "asset/asset_manager.hpp"
#include "asset/types/gfx_image.hpp"
#include "asset/types/material.hpp"
#include "asset/types/textureset.hpp"
#include "base_asset_converter.hpp"
#include "core/image_processing.hpp"
#include "shared/hash.hpp"

namespace PG
{

static std::shared_ptr<GfxImageCreateInfo> GetImgCreateInfoBase(
    const std::string& name, const std::shared_ptr<TexturesetCreateInfo>& texturesetInfo )
{
    auto imageCreateInfo             = std::make_shared<GfxImageCreateInfo>();
    imageCreateInfo->name            = name;
    imageCreateInfo->filenames[0]    = name;
    imageCreateInfo->clampHorizontal = texturesetInfo->clampHorizontal;
    imageCreateInfo->clampVertical   = texturesetInfo->clampVertical;
    imageCreateInfo->flipVertically  = texturesetInfo->flipVertically;
    return imageCreateInfo;
}

void MaterialConverter::AddReferencedAssetsInternal( ConstDerivedInfoPtr& matInfo )
{
    std::string texturesetName = matInfo->texturesetName.empty() ? "default" : matInfo->texturesetName;
    auto texturesetInfo        = AssetDatabase::FindAssetInfo<TexturesetCreateInfo>( ASSET_TYPE_TEXTURESET, texturesetName );

    {
        std::string albedoMetalnessName     = texturesetInfo->GetAlbedoMetalnessImageName( matInfo->applyAlbedo, matInfo->applyMetalness );
        auto imageCreateInfo                = GetImgCreateInfoBase( albedoMetalnessName, texturesetInfo );
        imageCreateInfo->semantic           = GfxImageSemantic::ALBEDO_METALNESS;
        imageCreateInfo->filenames[0]       = texturesetInfo->GetAlbedoMap( matInfo->applyAlbedo );
        imageCreateInfo->filenames[1]       = texturesetInfo->GetMetalnessMap( matInfo->applyMetalness );
        imageCreateInfo->compositeScales[1] = texturesetInfo->metalnessScale;
        imageCreateInfo->compositeSourceChannels[1] = texturesetInfo->metalnessSourceChannel;
        AddUsedAsset( ASSET_TYPE_GFX_IMAGE, imageCreateInfo );
    }

    {
        std::string normalRoughnessName     = texturesetInfo->GetNormalRoughnessImageName( matInfo->applyNormals, matInfo->applyRoughness );
        auto imageCreateInfo                = GetImgCreateInfoBase( normalRoughnessName, texturesetInfo );
        imageCreateInfo->semantic           = GfxImageSemantic::NORMAL_ROUGHNESS;
        imageCreateInfo->filenames[0]       = texturesetInfo->GetNormalMap( matInfo->applyNormals );
        imageCreateInfo->filenames[1]       = texturesetInfo->GetRoughnessMap( matInfo->applyRoughness );
        imageCreateInfo->compositeScales[0] = texturesetInfo->slopeScale;
        imageCreateInfo->compositeScales[1] = texturesetInfo->normalMapIsYUp;
        imageCreateInfo->compositeScales[2] = texturesetInfo->roughnessScale;
        imageCreateInfo->compositeScales[3] = texturesetInfo->invertRoughness;
        imageCreateInfo->compositeSourceChannels[1] = texturesetInfo->roughnessSourceChannel;
        AddUsedAsset( ASSET_TYPE_GFX_IMAGE, imageCreateInfo );
    }

    if ( matInfo->applyEmissive && !texturesetInfo->emissiveMap.empty() )
    {
        std::string emissiveMapName = texturesetInfo->GetEmissiveMap( matInfo->applyEmissive );
        auto imageCreateInfo        = GetImgCreateInfoBase( emissiveMapName, texturesetInfo );
        imageCreateInfo->semantic   = GfxImageSemantic::COLOR;
        AddUsedAsset( ASSET_TYPE_GFX_IMAGE, imageCreateInfo );
    }
}

std::string MaterialConverter::GetCacheNameInternal( ConstDerivedInfoPtr info )
{
    std::string cacheName = info->name;
    size_t hash           = Hash( info->albedoTint );
    HashCombine( hash, info->type );
    HashCombine( hash, info->metalnessTint );
    HashCombine( hash, info->roughnessTint );
    HashCombine( hash, info->emissiveTint );
    std::string texturesetName = info->texturesetName.empty() ? "default" : info->texturesetName;
    HashCombine( hash, texturesetName );
    auto texsetInfo = AssetDatabase::FindAssetInfo<TexturesetCreateInfo>( ASSET_TYPE_TEXTURESET, texturesetName );
    PG_ASSERT( texsetInfo );
    std::string albedoMetalness = texsetInfo->GetAlbedoMetalnessImageName( info->applyAlbedo, info->applyMetalness );
    HashCombine( hash, albedoMetalness );
    std::string normalRoughness = texsetInfo->GetNormalRoughnessImageName( info->applyNormals, info->applyRoughness );
    HashCombine( hash, normalRoughness );
    std::string emissiveMap = texsetInfo->GetEmissiveMap( info->applyEmissive );
    HashCombine( hash, emissiveMap );

    cacheName += "_" + std::to_string( hash );
    return cacheName;
}

AssetStatus MaterialConverter::IsAssetOutOfDateInternal( ConstDerivedInfoPtr matInfo, time_t materialCacheTimestamp )
{
    // if there is a cache entry at all, it should be up to date (since all fields + image names are part of the material hash name)
    return AssetStatus::UP_TO_DATE;
}

bool MaterialConverter::ConvertInternal( ConstDerivedInfoPtr& matInfo )
{
    std::string texturesetName = matInfo->texturesetName.empty() ? "default" : matInfo->texturesetName;
    auto texturesetInfo        = AssetDatabase::FindAssetInfo<TexturesetCreateInfo>( ASSET_TYPE_TEXTURESET, texturesetName );

    Material material;
    material.SetName( matInfo->name );
    material.type          = matInfo->type;
    material.albedoTint    = matInfo->albedoTint;
    material.metalnessTint = matInfo->metalnessTint;
    material.roughnessTint = matInfo->roughnessTint;
    material.emissiveTint  = matInfo->emissiveTint;
    material.albedoMetalnessImage =
        AssetManager::Get<GfxImage>( texturesetInfo->GetAlbedoMetalnessImageName( matInfo->applyAlbedo, matInfo->applyMetalness ) );
    material.normalRoughnessImage =
        AssetManager::Get<GfxImage>( texturesetInfo->GetNormalRoughnessImageName( matInfo->applyNormals, matInfo->applyRoughness ) );
    material.emissiveImage = nullptr;
    if ( matInfo->applyEmissive && !texturesetInfo->emissiveMap.empty() )
    {
        material.emissiveImage = AssetManager::Get<GfxImage>( texturesetInfo->GetEmissiveMap( matInfo->applyEmissive ) );
    }

    std::string cacheName = GetCacheName( matInfo );
    if ( !AssetCache::CacheAsset( assetType, cacheName, &material ) )
    {
        LOG_ERR( "Failed to cache asset %s %s (%s)", g_assetNames[assetType], matInfo->name.c_str(), cacheName.c_str() );
        return false;
    }

    return true;
}

} // namespace PG
