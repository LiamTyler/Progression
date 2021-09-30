#include "base_asset_converter.hpp"
#include "asset/types/material.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/hash.hpp"
#include "material_converter.hpp"
#include "shared/hash.hpp"

namespace PG
{

bool MaterialConverter::ParseInternal( const rapidjson::Value& value, InfoPtr info )
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
    mapping.ForEachMember( value, *info );

   return true;
}


std::string MaterialConverter::GetCacheNameInternal( ConstInfoPtr info )
{
    std::string cacheName = info->name;
    size_t hash = Hash( info->albedoTint );
    HashCombine( hash, info->metalnessTint );
    HashCombine( hash, info->roughnessTint );
    HashCombine( hash, info->albedoMapName );
    HashCombine( hash, info->metalnessMapName );
    HashCombine( hash, info->roughnessMapName );
    cacheName += "_" + std::to_string( hash );
    return cacheName;
}


ConvertDate MaterialConverter::IsAssetOutOfDateInternal( ConstInfoPtr info, time_t cacheTimestamp )
{
    // if cache name is found at all, it should be up to date
    return ConvertDate::UP_TO_DATE;
}

} // namespace PG