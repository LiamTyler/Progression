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

    ModelCreateInfo* info = (ModelCreateInfo*)baseInfo;
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

    ModelCreateInfo* info = (ModelCreateInfo*)baseInfo;
    std::string ffName = GetFastFileName( info );
    AddFastfileDependency( ffName );
    return IsFileOutOfDate( ffName, info->filename );
}


bool ModelConverter::ConvertSingle( const BaseAssetCreateInfo* baseInfo ) const
{
    ModelCreateInfo* info = (ModelCreateInfo*)baseInfo;
    LOG( "Converting model '%s'...", info->name.c_str() );
    Model model;
    std::vector< std::string > materialNames;
    if ( !Model_Load_PGModel( &model, *info, materialNames ) )
    {
        return false;
    }
    std::string fastfileName = GetFastFileName( info );
    Serializer serializer;
    if ( !serializer.OpenForWrite( fastfileName ) )
    {
        return false;
    }
    if ( !Fastfile_Model_Save( &model, &serializer, materialNames ) )
    {
        LOG_ERR( "Error while writing model '%s' to fastfile", model.name.c_str() );
        serializer.Close();
        DeleteFile( fastfileName );
        return false;
    }
    serializer.Close();

    return true;
}

} // namespace PG