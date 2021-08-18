#pragma once

#include "base_asset_converter.hpp"

namespace PG
{

class GfxImageConverter : public BaseAssetConverter
{
public:
    GfxImageConverter() : BaseAssetConverter( "Image", ASSET_TYPE_GFX_IMAGE ) {}
    void Parse( const rapidjson::Value& value ) override;

protected:
    std::string GetFastFileName( const BaseAssetCreateInfo* baseInfo ) const override;
    bool IsAssetOutOfDate( const BaseAssetCreateInfo* baseInfo ) override;
    bool ConvertSingle( const BaseAssetCreateInfo* baseInfo ) const override;
};

} // namespace PG