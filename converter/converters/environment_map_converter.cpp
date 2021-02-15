#include "environment_map_converter.hpp"
#include "asset/types/environment_map.hpp"
#include "asset/image.hpp"
#include "utils/hash.hpp"

namespace PG
{

void EnvironmentMapConverter::Parse( const rapidjson::Value& value )
{
    static JSONFunctionMapper< EnvironmentMapCreateInfo& > mapping(
    {
        { "name",           []( const rapidjson::Value& v, EnvironmentMapCreateInfo& s ) { s.name = v.GetString(); } },
        { "flipVertically", []( const rapidjson::Value& v, EnvironmentMapCreateInfo& s ) { s.flipVertically = v.GetBool(); } },
        { "flattenedCubemapFilename", []( const rapidjson::Value& v, EnvironmentMapCreateInfo& s ) { s.flattenedCubemapFilename = PG_ASSET_DIR + std::string( v.GetString() ); } },
        { "equirectangularFilename",  []( const rapidjson::Value& v, EnvironmentMapCreateInfo& s ) { s.equirectangularFilename  = PG_ASSET_DIR + std::string( v.GetString() ); } },
        { "left",   []( const rapidjson::Value& v, EnvironmentMapCreateInfo& s ) { s.faceFilenames[FACE_LEFT]   = PG_ASSET_DIR + std::string( v.GetString() ); } },
        { "right",  []( const rapidjson::Value& v, EnvironmentMapCreateInfo& s ) { s.faceFilenames[FACE_RIGHT]  = PG_ASSET_DIR + std::string( v.GetString() ); } },
        { "front",  []( const rapidjson::Value& v, EnvironmentMapCreateInfo& s ) { s.faceFilenames[FACE_FRONT]  = PG_ASSET_DIR + std::string( v.GetString() ); } },
        { "back",   []( const rapidjson::Value& v, EnvironmentMapCreateInfo& s ) { s.faceFilenames[FACE_BACK]   = PG_ASSET_DIR + std::string( v.GetString() ); } },
        { "top",    []( const rapidjson::Value& v, EnvironmentMapCreateInfo& s ) { s.faceFilenames[FACE_TOP]    = PG_ASSET_DIR + std::string( v.GetString() ); } },
        { "bottom", []( const rapidjson::Value& v, EnvironmentMapCreateInfo& s ) { s.faceFilenames[FACE_BOTTOM] = PG_ASSET_DIR + std::string( v.GetString() ); } },
    });

    EnvironmentMapCreateInfo* info = new EnvironmentMapCreateInfo;
    mapping.ForEachMember( value, *info );
    m_parsedAssets.push_back( info );
}


std::string EnvironmentMapConverter::GetFastFileName( const BaseAssetCreateInfo* baseInfo ) const
{
    EnvironmentMapCreateInfo* info = (EnvironmentMapCreateInfo*)baseInfo;
    std::string baseName = info->name;
    size_t fileHashes = 0;
    HashCombine( fileHashes, info->equirectangularFilename );
    HashCombine( fileHashes, info->flattenedCubemapFilename );
    for ( int i = 0; i < 6; ++i )
    {
        HashCombine( fileHashes, info->faceFilenames[i] );
    }
    baseName += "_" + std::to_string( fileHashes );
    baseName += "_v" + std::to_string( PG_ENVIRONMENT_MAP_VERSION );

    std::string fullName = PG_ASSET_DIR "cache/environment_maps/" + baseName + ".ffi";
    return fullName;
}


bool EnvironmentMapConverter::IsAssetOutOfDate( const BaseAssetCreateInfo* baseInfo )
{
    if ( g_converterConfigOptions.force )
    {
        return true;
    }

    EnvironmentMapCreateInfo* info = (EnvironmentMapCreateInfo*)baseInfo;
    std::string ffName = GetFastFileName( info );
    AddFastfileDependency( ffName );
    if ( !info->equirectangularFilename.empty() ) return IsFileOutOfDate( ffName, info->equirectangularFilename );
    else if ( !info->flattenedCubemapFilename.empty() ) return IsFileOutOfDate( ffName, info->flattenedCubemapFilename );
    else if ( !info->equirectangularFilename.empty() ) return IsFileOutOfDate( ffName, info->faceFilenames, 6 );
    else return true;
}


bool EnvironmentMapConverter::ConvertSingle( const BaseAssetCreateInfo* baseInfo ) const
{
    EnvironmentMapCreateInfo* info = (EnvironmentMapCreateInfo*)baseInfo;
    LOG( "Converting EnvironmentMap file '%s'...", info->name.c_str() );
    EnvironmentMap asset;
    if ( !EnvironmentMap_Load( &asset, *info ) )
    {
        return false;
    }
    std::string fastfileName = GetFastFileName( info );
    Serializer serializer;
    if ( !serializer.OpenForWrite( fastfileName ) )
    {
        return false;
    }
    if ( !Fastfile_EnvironmentMap_Save( &asset, &serializer ) )
    {
        LOG_ERR( "Error while writing EnvironmentMap '%s' to fastfile", info->name.c_str() );
        serializer.Close();
        DeleteFile( fastfileName );
        return false;
    }
    
    serializer.Close();

    return true;
}

} // namespace PG