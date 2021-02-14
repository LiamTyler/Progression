#include "converter.hpp"
#include "asset/types/environment_map.hpp"
#include "utils/hash.hpp"

using namespace PG;

extern void AddFastfileDependency( const std::string& file );

static std::vector< EnvironmentMapCreateInfo > s_parsedEnvironmentMaps;
static std::vector< EnvironmentMapCreateInfo > s_outOfDateEnvironmentMaps;

enum CubemapIndices
{
    INDEX_LEFT,
    INDEX_RIGHT,
    INDEX_FRONT,
    INDEX_BACK,
    INDEX_TOP,
    INDEX_BOTTOM,
};


void EnvironmentMap_Parse( const rapidjson::Value& value )
{

    static JSONFunctionMapper< EnvironmentMapCreateInfo& > mapping(
    {
        { "name",       []( const rapidjson::Value& v, EnvironmentMapCreateInfo& s ) { s.name = v.GetString(); } },
        { "filename",   []( const rapidjson::Value& v, EnvironmentMapCreateInfo& s ) { s.cubeFaceFilenames[0]            = PG_ASSET_DIR + std::string( v.GetString() ); } },
        { "left",       []( const rapidjson::Value& v, EnvironmentMapCreateInfo& s ) { s.cubeFaceFilenames[INDEX_LEFT]   = PG_ASSET_DIR + std::string( v.GetString() ); } },
        { "right",      []( const rapidjson::Value& v, EnvironmentMapCreateInfo& s ) { s.cubeFaceFilenames[INDEX_RIGHT]  = PG_ASSET_DIR + std::string( v.GetString() ); } },
        { "front",      []( const rapidjson::Value& v, EnvironmentMapCreateInfo& s ) { s.cubeFaceFilenames[INDEX_FRONT]  = PG_ASSET_DIR + std::string( v.GetString() ); } },
        { "back",       []( const rapidjson::Value& v, EnvironmentMapCreateInfo& s ) { s.cubeFaceFilenames[INDEX_BACK]   = PG_ASSET_DIR + std::string( v.GetString() ); } },
        { "top",        []( const rapidjson::Value& v, EnvironmentMapCreateInfo& s ) { s.cubeFaceFilenames[INDEX_TOP]    = PG_ASSET_DIR + std::string( v.GetString() ); } },
        { "bottom",     []( const rapidjson::Value& v, EnvironmentMapCreateInfo& s ) { s.cubeFaceFilenames[INDEX_BOTTOM] = PG_ASSET_DIR + std::string( v.GetString() ); } },
        { "flipVertically",      []( const rapidjson::Value& v, EnvironmentMapCreateInfo& s ) { s.flipVertically       = v.GetBool(); } },
        { "convertFromLDRtoHDR", []( const rapidjson::Value& v, EnvironmentMapCreateInfo& s ) { s.convertFromLDRtoHDR  = v.GetBool(); } },
    });

    EnvironmentMapCreateInfo info = {};
    mapping.ForEachMember( value, info );

    for ( int i = 0; i < 6; ++i )
    {
        if ( !PathExists( info.cubeFaceFilenames[i] ) )
        {
            LOG_ERR( "EnvironmentMap input file '%s' not found", info.cubeFaceFilenames[i].c_str() );
            g_converterStatus.parsingError = true;
        }
    }

    s_parsedEnvironmentMaps.push_back( info );
}


static std::string EnvironmentMap_GetFastFileName( const EnvironmentMapCreateInfo& info )
{
    std::string baseName = info.name;
    size_t fileHashes = 0;
    for ( int i = 0; i < 6; ++i )
    {
        HashCombine( fileHashes, info.cubeFaceFilenames[i] );
    }
    baseName += "_" + std::to_string( fileHashes );
    baseName += "_v" + std::to_string( PG_ENVIRONMENT_MAP_VERSION );

    std::string fullName = PG_ASSET_DIR "cache/environment_maps/" + baseName + ".ffi";
    return fullName;
}


static bool EnvironmentMap_IsOutOfDate( const EnvironmentMapCreateInfo& info )
{
    if ( g_converterConfigOptions.force )
    {
        return true;
    }

    std::string ffName = EnvironmentMap_GetFastFileName( info );
    AddFastfileDependency( ffName );
    return IsFileOutOfDate( ffName, info.cubeFaceFilenames, 6 );
}


static bool EnvironmentMap_ConvertSingle( const EnvironmentMapCreateInfo& info )
{
    LOG( "Converting EnvironmentMap file '%s'...", info.name.c_str() );
    EnvironmentMap asset;
    if ( !EnvironmentMap_Load( &asset, info ) )
    {
        return false;
    }
    std::string fastfileName = EnvironmentMap_GetFastFileName( info );
    Serializer serializer;
    if ( !serializer.OpenForWrite( fastfileName ) )
    {
        return false;
    }
    if ( !Fastfile_EnvironmentMap_Save( &asset, &serializer ) )
    {
        LOG_ERR( "Error while writing EnvironmentMap '%s' to fastfile", info.name.c_str() );
        serializer.Close();
        DeleteFile( fastfileName );
        return false;
    }
    
    serializer.Close();

    return true;
}


int EnvironmentMap_Convert()
{
    if ( s_outOfDateEnvironmentMaps.size() == 0 )
    {
        return 0;
    }

    int couldNotConvert = 0;
    for ( int i = 0; i < (int)s_outOfDateEnvironmentMaps.size(); ++i )
    {
        if ( !EnvironmentMap_ConvertSingle( s_outOfDateEnvironmentMaps[i] ) )
        {
            ++couldNotConvert;
        }
    }

    return couldNotConvert;
}


int EnvironmentMap_CheckDependencies()
{
    int outOfDate = 0;
    for ( size_t i = 0; i < s_parsedEnvironmentMaps.size(); ++i )
    {
        if ( EnvironmentMap_IsOutOfDate( s_parsedEnvironmentMaps[i] ) )
        {
            s_outOfDateEnvironmentMaps.push_back( s_parsedEnvironmentMaps[i] );
            ++outOfDate;
        }
    }

    return outOfDate;
}


bool EnvironmentMap_BuildFastFile( Serializer* serializer )
{
    for ( size_t i = 0; i < s_parsedEnvironmentMaps.size(); ++i )
    {
        std::string ffiName = EnvironmentMap_GetFastFileName( s_parsedEnvironmentMaps[i] );
        MemoryMapped inFile;
        if ( !inFile.open( ffiName ) )
        {
            LOG_ERR( "Could not open file '%s'", ffiName.c_str() );
            return false;
        }
        
        serializer->Write( AssetType::ASSET_TYPE_ENVIRONMENTMAP );
        serializer->Write( inFile.getData(), inFile.size() );
        inFile.close();
    }

    return true;
}