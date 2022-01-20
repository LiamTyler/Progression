#include "base_asset_converter.hpp"
#include "asset/types/material.hpp"
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
    HashCombine( hash, info->albedoMapName );
    HashCombine( hash, info->metalnessMapName );
    HashCombine( hash, info->roughnessMapName );
    cacheName += "_" + std::to_string( hash );
    return cacheName;
}


AssetStatus MaterialConverter::IsAssetOutOfDateInternal( ConstInfoPtr info, time_t cacheTimestamp )
{
    // if cache name is found at all, it should be up to date
    return AssetStatus::UP_TO_DATE;
}

} // namespace PG