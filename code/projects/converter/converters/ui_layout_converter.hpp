#pragma once

#include "asset/types/ui_layout.hpp"
#include "base_asset_converter.hpp"

namespace PG
{

class UILayoutConverter : public BaseAssetConverterTemplate<UILayout, UILayoutCreateInfo>
{
public:
    UILayoutConverter() : BaseAssetConverterTemplate( ASSET_TYPE_UI_LAYOUT ) {}

    virtual void AddReferencedAssetsInternal( ConstDerivedInfoPtr& baseInfo ) override;

protected:
    std::string GetCacheNameInternal( ConstDerivedInfoPtr info ) override;
    AssetStatus IsAssetOutOfDateInternal( ConstDerivedInfoPtr info, time_t cacheTimestamp ) override;
};

} // namespace PG