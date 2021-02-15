#pragma once

#include "converter.hpp"

class Serializer;

namespace PG
{

class MaterialFileConverter : public Converter
{
public:
    MaterialFileConverter() : Converter( "MatFile", ASSET_TYPE_MATERIAL ) {}
    void Parse( const rapidjson::Value& value ) override;

protected:
    std::string GetFastFileName( const BaseAssetCreateInfo* info ) const override;
    bool IsAssetOutOfDate( const BaseAssetCreateInfo* info ) override;
    bool ConvertSingle( const BaseAssetCreateInfo* info ) const override;
};

} // namespace PG