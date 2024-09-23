#pragma once

#include "asset/types/font.hpp"
#include "base_asset_converter.hpp"
#include "shared/json_parsing.hpp"

namespace PG
{

class FontConverter : public BaseAssetConverterTemplate<Font, FontCreateInfo>
{
public:
    FontConverter() : BaseAssetConverterTemplate( ASSET_TYPE_FONT ) {}

protected:
    std::string GetCacheNameInternal( ConstDerivedInfoPtr info ) override;
    AssetStatus IsAssetOutOfDateInternal( ConstDerivedInfoPtr info, time_t cacheTimestamp ) override;
    bool ConvertInternal( ConstDerivedInfoPtr& info ) override;
};

} // namespace PG
