#include "model_converter.hpp"

namespace PG
{

bool ModelConverter::ParseInternal( const rapidjson::Value& value, InfoPtr info )
{
    static JSONFunctionMapper<ModelCreateInfo&> mapping(
    {
        { "filename", []( const rapidjson::Value& v, ModelCreateInfo& i ) { i.filename = PG_ASSET_DIR + std::string( v.GetString() ); } },
    });
    mapping.ForEachMember( value, *info );
    return true;
}


std::string ModelConverter::GetCacheNameInternal( ConstInfoPtr info )
{
    std::string cacheName = info->name;
    cacheName += "_" + std::to_string( std::hash<std::string>{}( info->filename ) );
    return cacheName;
}


bool ModelConverter::IsAssetOutOfDateInternal( ConstInfoPtr info, time_t cacheTimestamp )
{
    AddFastfileDependency( info->filename );
    return IsFileOutOfDate( cacheTimestamp, info->filename );
}

} // namespace PG