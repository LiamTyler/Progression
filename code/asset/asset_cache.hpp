#pragma once

#include "asset/asset_versions.hpp"
#include "asset/types/base_asset.hpp"
#include <memory>
#include <string>

namespace PG::AssetCache
{

void Init();
time_t GetAssetTimestamp( AssetType assetType, const std::string& assetCacheName );
bool CacheAsset( AssetType assetType, const std::string& assetCacheName, BaseAsset* asset );
std::unique_ptr<char[]> GetCachedAssetRaw( AssetType assetType, const std::string& assetCacheName, size_t& numBytes );

} // namespace PG::AssetCache
