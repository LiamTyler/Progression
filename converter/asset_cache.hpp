#pragma once

#include "asset/asset_versions.hpp"
#include "asset/types/base_asset.hpp"
#include <string>


namespace PG::AssetCache
{

    void Init();
    time_t GetAssetTimestamp( AssetType assetType, const std::string& assetCacheName );
    bool CacheAsset( AssetType assetType, const std::string& assetCacheName, BaseAsset* asset );

} // namespace PG::AssetCache