#include "base_asset_converter.hpp"
#include "material_converter.hpp"
#include "asset/types/material.hpp"

namespace PG
{

std::shared_ptr<BaseAssetCreateInfo> MaterialConverter::Parse( const rapidjson::Value& value, std::shared_ptr<const BaseAssetCreateInfo> parent )
{
    using namespace rapidjson;
    static JSONFunctionMapper< MaterialCreateInfo& > mapping(
    {
        { "albedo",       []( const Value& v, MaterialCreateInfo& i ) { i.albedoTint       = ParseVec3( v ); } },
        { "metalness",    []( const Value& v, MaterialCreateInfo& i ) { i.metalnessTint    = ParseNumber<float>( v ); } },
        { "roughness",    []( const Value& v, MaterialCreateInfo& i ) { i.roughnessTint    = ParseNumber<float>( v ); } },
        { "albedoMap",    []( const Value& v, MaterialCreateInfo& i ) { i.albedoMapName    = v.GetString(); } },
        { "metalnessMap", []( const Value& v, MaterialCreateInfo& i ) { i.metalnessMapName = v.GetString(); } },
        { "roughnessMap", []( const Value& v, MaterialCreateInfo& i ) { i.roughnessMapName = v.GetString(); } },
    });

    auto info = std::make_shared<MaterialCreateInfo>();
    if ( parent )
    {
        *info = *std::static_pointer_cast<const MaterialCreateInfo>( parent );
    }
    mapping.ForEachMember( value, *info );

   return info;
}


std::string MaterialConverter::GetFastFileName( const BaseAssetCreateInfo* baseInfo ) const
{
    return "";
}


bool MaterialConverter::IsAssetOutOfDate( const BaseAssetCreateInfo* baseInfo )
{
    return true;
}

bool MaterialConverter::ConvertSingle( const BaseAssetCreateInfo* baseInfo ) const
{
    return false;
}

} // namespace PG