#include "converter.hpp"
#include "asset/types/material.hpp"

using namespace PG;

extern void AddFastfileDependency( const std::string& file );

static std::vector< std::string > s_parsedMaterialFilenames;
static std::vector< std::string > s_outOfDateMaterialFilenames;


void Material_Parse( const rapidjson::Value& value )
{

    static JSONFunctionMapper< std::string& > mapping(
    {
        { "filename",  []( const rapidjson::Value& v, std::string& filename ) { filename = PG_ASSET_DIR + std::string( v.GetString() ); } },
    });

    std::string pgMtlFilename;
    mapping.ForEachMember( value, pgMtlFilename );

    if ( !PathExists( pgMtlFilename ) )
    {
        LOG_ERR( "Material file '%s' not found", pgMtlFilename.c_str() );
        g_converterStatus.parsingError = true;
    }

    s_parsedMaterialFilenames.push_back( pgMtlFilename );
}


static std::string Material_GetFastFileName( const std::string& filename )
{
    std::string baseName = GetFilenameStem( filename );
    baseName += "_v" + std::to_string( PG_MATERIAL_VERSION );

    std::string fullName = PG_ASSET_DIR "cache/materials/" + baseName + ".ffi";
    return fullName;
}


static bool Material_IsOutOfDate( const std::string& filename )
{
    if ( g_converterConfigOptions.force )
    {
        return true;
    }

    std::string ffName = Material_GetFastFileName( filename );
    AddFastfileDependency( ffName );
    return IsFileOutOfDate( ffName, filename );
}


static bool ParseMaterialFile( const std::string& filename, std::vector< MaterialCreateInfo >& createInfos )
{
    using namespace rapidjson;
    Document document;
    if ( !ParseJSONFile( filename, document ) )
    {
        return false;
    }
    
    static JSONFunctionMapper< MaterialCreateInfo& > mapping(
    {
        { "name",        []( const Value& v, MaterialCreateInfo& i ) { i.name        = v.GetString(); } },
        { "Kd",          []( const Value& v, MaterialCreateInfo& i ) { i.Kd          = ParseVec3( v ); } },
        { "map_Kd_name", []( const Value& v, MaterialCreateInfo& i ) { i.map_Kd_name = v.GetString(); } },
    });
    
    PG_ASSERT( document.HasMember( "Materials" ), "material file requires a single object 'Materials' that has a list of all material objects" );
    const auto& materialList = document["Materials"];
    for ( Value::ConstValueIterator itr = materialList.Begin(); itr != materialList.End(); ++itr )
    {
        MaterialCreateInfo createInfo;
        mapping.ForEachMember( *itr, createInfo );
        createInfos.push_back( createInfo );
    }

    return true;
}


static bool Material_ConvertSingle( const std::string& filename )
{
    LOG( "Converting material file '%s'...\n", filename.c_str() );
    std::vector< MaterialCreateInfo > createInfos;
    if ( !ParseMaterialFile( filename, createInfos ) )
    {
        return false;
    }
    
    std::string fastfileName = Material_GetFastFileName( filename );
    Serializer serializer;
    if ( !serializer.OpenForWrite( fastfileName ) )
    {
        return false;
    }
    uint16_t numMaterials = static_cast< uint16_t >( createInfos.size() );
    serializer.Write( numMaterials );
    for ( const auto& info : createInfos )
    {
        if ( !Fastfile_Material_Save( &info, &serializer ) )
        {
            LOG_ERR( "Error while writing material '%s' to fastfile\n", info.name.c_str() );
            serializer.Close();
            DeleteFile( fastfileName );
            return false;
        }
    }
    
    serializer.Close();

    return true;
}


int Material_Convert()
{
    if ( s_outOfDateMaterialFilenames.size() == 0 )
    {
        return 0;
    }

    int couldNotConvert = 0;
    for ( int i = 0; i < (int)s_outOfDateMaterialFilenames.size(); ++i )
    {
        if ( !Material_ConvertSingle( s_outOfDateMaterialFilenames[i] ) )
        {
            ++couldNotConvert;
        }
    }

    return couldNotConvert;
}


int Material_CheckDependencies()
{
    int outOfDate = 0;
    for ( size_t i = 0; i < s_parsedMaterialFilenames.size(); ++i )
    {
        if ( Material_IsOutOfDate( s_parsedMaterialFilenames[i] ) )
        {
            s_outOfDateMaterialFilenames.push_back( s_parsedMaterialFilenames[i] );
            ++outOfDate;
        }
    }

    return outOfDate;
}


bool Material_BuildFastFile( Serializer* serializer )
{
    for ( size_t i = 0; i < s_parsedMaterialFilenames.size(); ++i )
    {
        std::string ffiName = Material_GetFastFileName( s_parsedMaterialFilenames[i] );
        MemoryMapped inFile;
        if ( !inFile.open( ffiName ) )
        {
            LOG_ERR( "Could not open file '%s'\n", ffiName.c_str() );
            return false;
        }
        
        serializer->Write( AssetType::ASSET_TYPE_MATERIAL );
        serializer->Write( inFile.getData(), inFile.size() );
        inFile.close();
    }

    return true;
}