#include "model_converter.hpp"
#include "asset/types/model.hpp"

namespace PG
{

void ModelConverter::Parse( const rapidjson::Value& value )
{
    static JSONFunctionMapper< ModelCreateInfo& > mapping(
    {
        { "name",      []( const rapidjson::Value& v, ModelCreateInfo& i ) { i.name = v.GetString(); } },
        { "filename",  []( const rapidjson::Value& v, ModelCreateInfo& i ) { i.filename = PG_ASSET_DIR + std::string( v.GetString() ); } },
    });

    ModelCreateInfo* info = new ModelCreateInfo;
    mapping.ForEachMember( value, *info );

    if ( !PathExists( info->filename ) )
    {
        LOG_ERR( "Filename '%s' not found for Model '%s', skipping model", info->filename.c_str(), info->name.c_str() );
        g_converterStatus.parsingError = true;
    }

    m_parsedAssets.push_back( info );
}


std::string ModelConverter::GetFastFileName( const BaseAssetCreateInfo* baseInfo ) const
{
    static_assert( sizeof( ModelCreateInfo ) == 2 * sizeof( std::string ), "Dont forget to add new hash value" );

    const ModelCreateInfo* info = (const ModelCreateInfo*)baseInfo;
    std::string baseName = info->name;
    baseName += "_v" + std::to_string( PG_GFX_IMAGE_VERSION );
    baseName += "_" + std::to_string( std::hash< std::string >{}( info->filename ) );

    std::string fullName = PG_ASSET_DIR "cache/models/" + baseName + ".ffi";
    return fullName;
}


bool ModelConverter::IsAssetOutOfDate( const BaseAssetCreateInfo* baseInfo )
{
    if ( g_converterConfigOptions.force )
    {
        return true;
    }

    const ModelCreateInfo* info = (const ModelCreateInfo*)baseInfo;
    std::string ffName = GetFastFileName( info );
    AddFastfileDependency( ffName );
    return IsFileOutOfDate( ffName, info->filename );
}


bool ModelConverter::ConvertSingle( const BaseAssetCreateInfo* baseInfo ) const
{
    return ConvertSingleInternal< Model, ModelCreateInfo >( baseInfo );
}

} // namespace PG