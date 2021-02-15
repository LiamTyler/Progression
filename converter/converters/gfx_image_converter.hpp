#pragma once

#include "converter.hpp"

class Serializer;

namespace PG
{

class GfxImageConverter : public Converter
{
public:
    GfxImageConverter() : Converter( "Image", ASSET_TYPE_GFX_IMAGE ) {}
    void Parse( const rapidjson::Value& value ) override;

protected:
    std::string GetFastFileName( const BaseAssetCreateInfo* baseInfo ) const override;
    bool IsAssetOutOfDate( const BaseAssetCreateInfo* baseInfo ) override;
    bool ConvertSingle( const BaseAssetCreateInfo* baseInfo ) const override;
};

} // namespace PG