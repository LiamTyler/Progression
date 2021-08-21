#pragma once

#include "asset/asset_versions.hpp"
#include "asset/types/base_asset.hpp"
#include <memory>

namespace PG::AssetDatabase
{

void Init();

std::shared_ptr<BaseAssetCreateInfo> FindAssetInfo( AssetType type, const std::string& name );

} // namespace PG::AssetDatabase