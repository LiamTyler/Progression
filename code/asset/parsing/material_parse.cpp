#include "material_parse.hpp"

namespace PG
{

bool MaterialParser::ParseInternal( const rapidjson::Value& value, DerivedInfoPtr info )
{
    using namespace rapidjson;
    static JSONFunctionMapper< MaterialCreateInfo& > mapping(
    {
        { "textureset", []( const Value& v, MaterialCreateInfo& i ) { i.texturesetName = v.GetString(); } },
        { "albedo",     []( const Value& v, MaterialCreateInfo& i ) { i.albedoTint     = ParseVec3( v ); } },
        { "metalness",  []( const Value& v, MaterialCreateInfo& i ) { i.metalnessTint  = ParseNumber<float>( v ); } },
        //{ "roughness",  []( const Value& v, MaterialCreateInfo& i ) { i.roughnessTint  = ParseNumber<float>( v ); } },
    });
    mapping.ForEachMember( value, *info );

   return true;
}

} // namespace PG