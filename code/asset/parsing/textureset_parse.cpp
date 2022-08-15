#include "textureset_parse.hpp"
#include "core/feature_defines.hpp"

namespace PG
{

BEGIN_STR_TO_ENUM_MAP( Channel )
    STR_TO_ENUM_VALUE( Channel, R )
    STR_TO_ENUM_VALUE( Channel, G )
    STR_TO_ENUM_VALUE( Channel, B )
    STR_TO_ENUM_VALUE( Channel, A )
END_STR_TO_ENUM_MAP( Channel, R )

bool TexturesetParser::ParseInternal( const rapidjson::Value& value, DerivedInfoPtr info )
{
    static JSONFunctionMapper<TexturesetCreateInfo&> mapping(
    {
        { "name", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.name = v.GetString(); } },
        { "clampHorizontal", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.clampHorizontal = v.GetBool(); } },
        { "clampVertical", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.clampVertical = v.GetBool(); } },
        { "albedoMap", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.albedoMap = v.GetString(); } },
        { "metalnessMap", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.metalnessMap = v.GetString(); } },
        { "metalnessSourceChannel", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.metalnessSourceChannel = Channel_StringToEnum( v.GetString() ); } },
        //{ "normalMap", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.normalMap = v.GetString(); } },
        //{ "slopeScale", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.slopeScale = ParseNumber<float>( v ); } },
        //{ "roughnessMap", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.roughnessMap = v.GetString(); } },
        //{ "roughnessSourceChannel", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.roughnessSourceChannel = Channel_StringToEnum( v.GetString() ); } },
        //{ "invertRoughness", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.invertRoughness = v.GetBool(); } },
    });
    mapping.ForEachMember( value, *info );

    return true;
}

} // namespace PG