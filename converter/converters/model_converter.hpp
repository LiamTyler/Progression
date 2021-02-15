#pragma once

#include "converter.hpp"

namespace PG
{

class ModelConverter : public Converter
{
public:
    ModelConverter() : Converter( "Model", ASSET_TYPE_MODEL ) {}
    void Parse( const rapidjson::Value& value ) override;

protected:
    std::string GetFastFileName( const BaseAssetCreateInfo* baseInfo ) const override;
    bool IsAssetOutOfDate( const BaseAssetCreateInfo* baseInfo ) override;
    bool ConvertSingle( const BaseAssetCreateInfo* baseInfo ) const override;
};

} // namespace PG