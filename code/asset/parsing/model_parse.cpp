#include "model_parse.hpp"

namespace PG
{

bool ModelParser::ParseInternal( const rapidjson::Value& value, InfoPtr info )
{
    static JSONFunctionMapper<ModelCreateInfo&> mapping(
    {
        { "filename", []( const rapidjson::Value& v, ModelCreateInfo& i ) { i.filename = PG_ASSET_DIR + std::string( v.GetString() ); } },
    });
    mapping.ForEachMember( value, *info );

    if ( info->filename.empty() )
    {
        LOG_ERR( "Must specify a filename for Model asset '%s'", info->name.c_str() );
        return false;
    }

    return true;
}

} // namespace PG