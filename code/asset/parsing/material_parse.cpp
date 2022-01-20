#include "material_parse.hpp"

namespace PG
{

bool MaterialParser::ParseInternal( const rapidjson::Value& value, InfoPtr info )
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

} // namespace PG