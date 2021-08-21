#pragma once

#include "asset/asset_versions.hpp"
#include <string>


namespace PG
{

class BaseAsset;

class AssetCache
{
public:
    AssetCache();

    bool IsAssetOutOfDate( AssetType assetType, const std::string& assetCacheName );

    void CacheAsset( BaseAsset* asset );
};

} // namespace PG