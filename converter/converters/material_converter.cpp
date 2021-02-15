#include "converter.hpp"
#include "material_converter.hpp"
#include "asset/types/material.hpp"

namespace PG
{

struct MaterialFileCreateInfo : public BaseAssetCreateInfo
{
    std::string filename;
};

void MaterialFileConverter::Parse( const rapidjson::Value& value )
{
    static JSONFunctionMapper< MaterialFileCreateInfo& > mapping(
    {
        { "filename",  []( const rapidjson::Value& v, MaterialFileCreateInfo& info ) { info.filename = PG_ASSET_DIR + std::string( v.GetString() ); } },
    });

    MaterialFileCreateInfo* info = new MaterialFileCreateInfo;
    mapping.ForEachMember( value, *info );

    if ( !PathExists( info->filename ) )
    {
        LOG_ERR( "Material file '%s' not found", info->filename.c_str() );
        g_converterStatus.parsingError = true;
    }

    m_parsedAssets.push_back( info );
}


std::string MaterialFileConverter::GetFastFileName( const BaseAssetCreateInfo* baseInfo ) const
{
    const MaterialFileCreateInfo* info = (const MaterialFileCreateInfo*)baseInfo;
    std::string baseName = GetFilenameStem( info->filename );
    baseName += "_v" + std::to_string( PG_MATERIAL_VERSION );

    std::string fullName = PG_ASSET_DIR "cache/materials/" + baseName + ".ffi";
    return fullName;
}


bool MaterialFileConverter::IsAssetOutOfDate( const BaseAssetCreateInfo* baseInfo )
{
    if ( g_converterConfigOptions.force )
    {
        return true;
    }

    const MaterialFileCreateInfo* info = (const MaterialFileCreateInfo*)baseInfo;
    std::string ffName = GetFastFileName( info );
    AddFastfileDependency( ffName );
    return IsFileOutOfDate( ffName, info->filename );
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
        { "name",         []( const Value& v, MaterialCreateInfo& i ) { i.name             = v.GetString(); } },
        { "albedo",       []( const Value& v, MaterialCreateInfo& i ) { i.albedoTint           = ParseVec3( v ); } },
        { "metalness",    []( const Value& v, MaterialCreateInfo& i ) { i.metalnessTint        = ParseNumber< float >( v ); } },
        { "roughness",    []( const Value& v, MaterialCreateInfo& i ) { i.roughnessTint        = ParseNumber< float >( v ); } },
        { "albedoMap",    []( const Value& v, MaterialCreateInfo& i ) { i.albedoMapName    = v.GetString(); } },
        { "metalnessMap", []( const Value& v, MaterialCreateInfo& i ) { i.metalnessMapName = v.GetString(); } },
        { "roughnessMap", []( const Value& v, MaterialCreateInfo& i ) { i.roughnessMapName = v.GetString(); } },
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


bool MaterialFileConverter::ConvertSingle( const BaseAssetCreateInfo* baseInfo ) const
{
    const MaterialFileCreateInfo* info = (const MaterialFileCreateInfo*)baseInfo;
    LOG( "Converting MatFile '%s'...", info->filename.c_str() );
    std::vector< MaterialCreateInfo > createInfos;
    if ( !ParseMaterialFile( info->filename, createInfos ) )
    {
        return false;
    }
    
    std::string fastfileName = GetFastFileName( info );
    try
    {
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
                LOG_ERR( "Error while writing material '%s' to fastfile", info.name.c_str() );
                serializer.Close();
                DeleteFile( fastfileName );
                return false;
            }
        }
        serializer.Close();
    }
    catch ( std::exception& e )
    {
        DeleteFile( fastfileName );
        throw e;
    }

    return true;
}

} // namespace PG