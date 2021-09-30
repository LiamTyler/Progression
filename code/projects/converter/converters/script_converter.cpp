#include "script_converter.hpp"

namespace PG
{

bool ScriptConverter::ParseInternal( const rapidjson::Value& value, InfoPtr info )
{
    static JSONFunctionMapper<ScriptCreateInfo&> mapping(
    {
        { "filename", []( const rapidjson::Value& v, ScriptCreateInfo& s ) { s.filename = PG_ASSET_DIR + std::string( v.GetString() ); } },
    });
    mapping.ForEachMember( value, *info );
    return true;
}


std::string ScriptConverter::GetCacheNameInternal( ConstInfoPtr info )
{
    std::string cacheName = info->name;
    cacheName += "_" + GetFilenameStem( info->filename );
    return cacheName;
}


ConvertDate ScriptConverter::IsAssetOutOfDateInternal( ConstInfoPtr info, time_t cacheTimestamp )
{
    AddFastfileDependency( info->filename );
    return IsFileOutOfDate( cacheTimestamp, info->filename ) ? ConvertDate::OUT_OF_DATE : ConvertDate::UP_TO_DATE;
}

} // namespace PG