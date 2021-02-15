#pragma once

#include "converter.hpp"

class Serializer;

namespace PG
{

class ShaderConverter : public Converter
{
public:
    ShaderConverter () : Converter( "Shader", ASSET_TYPE_SHADER ) {}
    void Parse( const rapidjson::Value& value ) override;

protected:
    std::string GetFastFileName( const BaseAssetCreateInfo* info ) const override;
    bool IsAssetOutOfDate( const BaseAssetCreateInfo* info ) override;
    bool ConvertSingle( const BaseAssetCreateInfo* info ) const override;
};

} // namespace PG