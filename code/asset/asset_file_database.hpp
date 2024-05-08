#pragma once

#include "asset/asset_versions.hpp"
#include "asset/types/base_asset.hpp"
#include <memory>

namespace PG::AssetDatabase
{

bool Init();

std::shared_ptr<BaseAssetCreateInfo> FindAssetInfo( AssetType type, const std::string& name );

template <typename DerivedAssetCreateInfo>
std::shared_ptr<DerivedAssetCreateInfo> FindAssetInfo( AssetType type, const std::string& name )
{
    return std::static_pointer_cast<DerivedAssetCreateInfo>( FindAssetInfo( type, name ) );
}

} // namespace PG::AssetDatabase
