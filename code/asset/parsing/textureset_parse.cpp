#include "textureset_parse.hpp"
#include "core/feature_defines.hpp"

namespace PG
{

BEGIN_STR_TO_ENUM_MAP( ChannelSelect )
    STR_TO_ENUM_VALUE( ChannelSelect, R )
    STR_TO_ENUM_VALUE( ChannelSelect, G )
    STR_TO_ENUM_VALUE( ChannelSelect, B )
    STR_TO_ENUM_VALUE( ChannelSelect, A )
END_STR_TO_ENUM_MAP( ChannelSelect, R )

bool TexturesetParser::ParseInternal( const rapidjson::Value& value, InfoPtr info )
{
    static JSONFunctionMapper<TexturesetCreateInfo&> mapping(
    {
        { "name", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.name = v.GetString(); } },
        { "clampU", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.clampU = v.GetBool(); } },
        { "clampV", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.clampV = v.GetBool(); } },
        { "albedoMap", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.albedoMap = v.GetString(); } },
        { "metalnessMap", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.metalnessMap = v.GetString(); } },
        { "metalnessSourceChannel", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.metalnessSourceChannel = ChannelSelect_StringToEnum( v.GetString() ); } },
        { "normalMap", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.normalMap = v.GetString(); } },
        { "slopeScale", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.slopeScale = ParseNumber<float>( v ); } },
        { "roughnessMap", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.roughnessMap = v.GetString(); } },
        { "roughnessSourceChannel", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.roughnessSourceChannel = ChannelSelect_StringToEnum( v.GetString() ); } },
        { "invertRoughness", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.invertRoughness = v.GetBool(); } },
    });
    mapping.ForEachMember( value, *info );

    return true;
}

} // namespace PG