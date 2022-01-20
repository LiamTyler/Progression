#include "script_parse.hpp"

namespace PG
{

bool ScriptParser::ParseInternal( const rapidjson::Value& value, InfoPtr info )
{
    static JSONFunctionMapper<ScriptCreateInfo&> mapping(
    {
        { "filename", []( const rapidjson::Value& v, ScriptCreateInfo& s ) { s.filename = PG_ASSET_DIR + std::string( v.GetString() ); } },
    });
    mapping.ForEachMember( value, *info );
    return true;
}

} // namespace PG