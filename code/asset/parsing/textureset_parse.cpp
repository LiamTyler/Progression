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

/*
* struct TexturesetCreateInfo : public BaseAssetCreateInfo
{
    std::string albedoMap;

    std::string metalnessMap;
    ChannelSelect metalnessSourceChannel = ChannelSelect::R;
    float metalnessScale = 1.0f;

    std::string normalMap;
    float slopeScale = 1.0f;

    std::string roughnessMap;
    ChannelSelect roughnessSourceChannel = ChannelSelect::R;
    bool invertRoughness = false; // if the source map is actually a gloss map
};
*/

bool TexturesetParser::ParseInternal( const rapidjson::Value& value, InfoPtr info )
{
    static JSONFunctionMapper<TexturesetCreateInfo&> mapping(
    {
        { "name",         []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.name = v.GetString(); } },
        { "albedoMap",    []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.albedoMap = v.GetString(); } },
        { "metalnessMap", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.metalnessMap = v.GetString(); } },
        { "metalnessSourceChannel", []( const rapidjson::Value& v, TexturesetCreateInfo& s ) { s.metalnessSourceChannel = ChannelSelect_StringToEnum( v.GetString() ); } },
    });
    mapping.ForEachMember( value, *info );

    return true;
}

} // namespace PG