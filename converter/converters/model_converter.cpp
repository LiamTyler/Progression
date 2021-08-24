#include "model_converter.hpp"
#include "asset/types/model.hpp"

namespace PG
{

std::shared_ptr<BaseAssetCreateInfo> ModelConverter::Parse( const rapidjson::Value& value, std::shared_ptr<const BaseAssetCreateInfo> parent )
{
    static JSONFunctionMapper< ModelCreateInfo& > mapping(
    {
        { "name",      []( const rapidjson::Value& v, ModelCreateInfo& i ) { i.name = v.GetString(); } },
        { "filename",  []( const rapidjson::Value& v, ModelCreateInfo& i ) { i.filename = PG_ASSET_DIR + std::string( v.GetString() ); } },
    });

    auto info = std::make_shared<ModelCreateInfo>();
    if ( parent )
    {
        *info = *std::static_pointer_cast<const ModelCreateInfo>( parent );
    }
    mapping.ForEachMember( value, *info );

    return info;
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