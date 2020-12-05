#include "converter.hpp"
#include "asset/types/model.hpp"

using namespace PG;

extern void AddFastfileDependency( const std::string& file );

static std::vector< ModelCreateInfo > s_parsedModels;
static std::vector< ModelCreateInfo > s_outOfDateModels;

void Model_Parse( const rapidjson::Value& value )
{
    static JSONFunctionMapper< ModelCreateInfo& > mapping(
    {
        { "name",      []( const rapidjson::Value& v, ModelCreateInfo& i ) { i.name = v.GetString(); } },
        { "filename",  []( const rapidjson::Value& v, ModelCreateInfo& i ) { i.filename = PG_ASSET_DIR + std::string( v.GetString() ); } },
    });

    ModelCreateInfo info;
    mapping.ForEachMember( value, info );

    if ( !PathExists( info.filename ) )
    {
        LOG_ERR( "Filename '%s' not found for Model '%s', skipping model", info.filename.c_str(), info.name.c_str() );
        g_converterStatus.parsingError = true;
    }

    s_parsedModels.push_back( info );
}


static std::string Model_GetFastFileName( const ModelCreateInfo& info )
{
    static_assert( sizeof( ModelCreateInfo ) == 2 * sizeof( std::string ), "Dont forget to add new hash value" );

    std::string baseName = info.name;
    baseName += "_v" + std::to_string( PG_GFX_IMAGE_VERSION );
    baseName += "_" + std::to_string( std::hash< std::string >{}( info.filename ) );

    std::string fullName = PG_ASSET_DIR "cache/models/" + baseName + ".ffi";
    return fullName;
}


static bool Model_IsOutOfDate( const ModelCreateInfo& info )
{
    if ( g_converterConfigOptions.force )
    {
        return true;
    }

    std::string ffName = Model_GetFastFileName( info );
    AddFastfileDependency( ffName );
    return IsFileOutOfDate( ffName, info.filename );
}


static bool Model_ConvertSingle( const ModelCreateInfo& info )
{
    LOG( "Converting model '%s'...\n", info.name.c_str() );
    Model model;
    std::vector< std::string > materialNames;
    if ( !Model_Load_PGModel( &model, info, materialNames ) )
    {
        return false;
    }
    std::string fastfileName = Model_GetFastFileName( info );
    Serializer serializer;
    if ( !serializer.OpenForWrite( fastfileName ) )
    {
        return false;
    }
    if ( !Fastfile_Model_Save( &model, &serializer, materialNames ) )
    {
        LOG_ERR( "Error while writing model '%s' to fastfile\n", model.name.c_str() );
        serializer.Close();
        DeleteFile( fastfileName );
        return false;
    }
    serializer.Close();

    return true;
}


int Model_CheckDependencies()
{
    int outOfDate = 0;
    for ( size_t i = 0; i < s_parsedModels.size(); ++i )
    {
        if ( Model_IsOutOfDate( s_parsedModels[i] ) )
        {
            s_outOfDateModels.push_back( s_parsedModels[i] );
            ++outOfDate;
        }
    }

    return outOfDate;
}


int Model_Convert()
{
    if ( s_outOfDateModels.size() == 0 )
    {
        return 0;
    }

    int couldNotConvert = 0;
    for ( int i = 0; i < (int)s_outOfDateModels.size(); ++i )
    {
        if ( !Model_ConvertSingle( s_outOfDateModels[i] ) )
        {
            ++couldNotConvert;
        }
    }

    return couldNotConvert;
}


bool Model_BuildFastFile( Serializer* serializer )
{
    for ( size_t i = 0; i < s_parsedModels.size(); ++i )
    {
        std::string ffiName = Model_GetFastFileName( s_parsedModels[i] );
        MemoryMapped inFile;
        if ( !inFile.open( ffiName ) )
        {
            LOG_ERR( "Could not open file '%s'\n", ffiName.c_str() );
            return false;
        }
        
        serializer->Write( AssetType::ASSET_TYPE_MODEL );
        serializer->Write( inFile.getData(), inFile.size() );
        inFile.close();
    }

    return true;
}