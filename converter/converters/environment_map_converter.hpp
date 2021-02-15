#pragma once

#include "converter.hpp"

class Serializer;

namespace PG
{

class EnvironmentMapConverter : public Converter
{
public:
    EnvironmentMapConverter() : Converter( "EnvironmentMap", ASSET_TYPE_ENVIRONMENTMAP ) {}
    void Parse( const rapidjson::Value& value ) override;

protected:
    std::string GetFastFileName( const BaseAssetCreateInfo* baseInfo ) const override;
    bool IsAssetOutOfDate( const BaseAssetCreateInfo* baseInfo ) override;
    bool ConvertSingle( const BaseAssetCreateInfo* baseInfo ) const override;
};

} // namespace PG