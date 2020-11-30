#include "core/assert.hpp"
#include "asset/shader_preprocessor.hpp"
#include "asset/types/shader.hpp"
#include "asset/asset_versions.hpp"
#include "utils/filesystem.hpp"
#include "utils/file_dependency.hpp"
#include "utils/json_parsing.hpp"
#include "utils/logger.hpp"
#include "utils/serializer.hpp"

using namespace PG;

extern void AddFastfileDependency( const std::string& file );

std::vector< ShaderCreateInfo > g_parsedShaders;
std::vector< ShaderCreateInfo > g_outOfDateShaders;
extern bool g_parsingError;
extern int g_checkDependencyErrors;


static std::unordered_map< std::string, ShaderStage > shaderStageMap =
{
    { "vertex",                  ShaderStage::VERTEX },
    { "tessellation_control",    ShaderStage::TESSELLATION_CONTROL },
    { "tessellation_evaluation", ShaderStage::TESSELLATION_EVALUATION },
    { "geometry",                ShaderStage::GEOMETRY },
    { "fragment",                ShaderStage::FRAGMENT },
    { "compute",                 ShaderStage::COMPUTE },
};


void Shader_Parse( const rapidjson::Value& value )
{
    static JSONFunctionMapper< ShaderCreateInfo& > mapping(
    {
        { "name",        []( const rapidjson::Value& v, ShaderCreateInfo& s ) { s.name        = v.GetString(); } },
        { "filename",    []( const rapidjson::Value& v, ShaderCreateInfo& s ) { s.filename = PG_ASSET_DIR "shaders/" + std::string( v.GetString() ); } },
        { "shaderStage", []( const rapidjson::Value& v, ShaderCreateInfo& s )
            {
                std::string stageName = v.GetString();
                auto it = shaderStageMap.find( stageName );
                if ( it == shaderStageMap.end() )
                {
                    LOG_ERR( "No ShaderStage found matching '%s'\n", stageName.c_str() );
                    s.shaderStage = ShaderStage::NUM_SHADER_STAGES;
                }
                else
                {
                    s.shaderStage = it->second;
                }
            }
        },
    });

    ShaderCreateInfo info;
    info.shaderStage = ShaderStage::NUM_SHADER_STAGES;
    mapping.ForEachMember( value, info );

    if ( !PathExists( info.filename ) )
    {
        LOG_ERR( "Shader file '%s' not found\n", info.filename.c_str() );
        g_parsingError = true;
    }

    if ( info.shaderStage == ShaderStage::NUM_SHADER_STAGES )
    {
        LOG_ERR( "Shader '%s' must have a valid ShaderStage!\n", info.name.c_str() );
        g_parsingError = true;
    }

    g_parsedShaders.push_back( info );
}


static std::string Shader_GetFastFileName( const ShaderCreateInfo& info )
{
    std::string baseName = info.name;
    baseName += "_" + GetRelativeFilename( info.filename );
    baseName += "_v" + std::to_string( PG_SHADER_VERSION );

    std::string fullName = PG_ASSET_DIR "cache/shaders/" + baseName + ".ffi";
    return fullName;
}


static bool Shader_IsOutOfDate( const ShaderCreateInfo& info )
{
    std::string ffName = Shader_GetFastFileName( info );
    ShaderPreprocessOutput preproc = PreprocessShaderForIncludeListOnly( info.filename, info.defines );
    if ( !preproc.success )
    {
        LOG_ERR( "Preprocessing shader asset '%s' for the included files failed\n", info.name.c_str() );
        g_checkDependencyErrors += 1;
        return false;
    }

    AddFastfileDependency( ffName );
    for ( const auto& file : preproc.includedFiles )
    {
        AddFastfileDependency( file );
    }

    return IsFileOutOfDate( ffName, info.filename ) || IsFileOutOfDate( ffName, preproc.includedFiles );
}


static bool Shader_ConvertSingle( const ShaderCreateInfo& info )
{
    LOG( "Converting Shader file '%s'...\n", info.filename.c_str() );
    Shader asset;
    if ( !Shader_Load( &asset, info ) )
    {
        return false;
    }
    std::string fastfileName = Shader_GetFastFileName( info );
    Serializer serializer;
    if ( !serializer.OpenForWrite( fastfileName ) )
    {
        return false;
    }
    if ( !Fastfile_Shader_Save( &asset, &serializer ) )
    {
        LOG_ERR( "Error while writing Shader '%s' to fastfile\n", info.name.c_str() );
        serializer.Close();
        DeleteFile( fastfileName );
        return false;
    }
    
    serializer.Close();

    return true;
}


int Shader_Convert()
{
    if ( g_outOfDateShaders.size() == 0 )
    {
        return 0;
    }

    int couldNotConvert = 0;
    for ( int i = 0; i < (int)g_outOfDateShaders.size(); ++i )
    {
        if ( !Shader_ConvertSingle( g_outOfDateShaders[i] ) )
        {
            ++couldNotConvert;
        }
    }

    return couldNotConvert;
}


int Shader_CheckDependencies()
{
    int outOfDate = 0;
    for ( size_t i = 0; i < g_parsedShaders.size(); ++i )
    {
        if ( Shader_IsOutOfDate( g_parsedShaders[i] ) )
        {
            g_outOfDateShaders.push_back( g_parsedShaders[i] );
            ++outOfDate;
        }
    }

    return outOfDate;
}


bool Shader_BuildFastFile( Serializer* serializer )
{
    for ( size_t i = 0; i < g_parsedShaders.size(); ++i )
    {
        std::string ffiName = Shader_GetFastFileName( g_parsedShaders[i] );
        MemoryMapped inFile;
        if ( !inFile.open( ffiName ) )
        {
            LOG_ERR( "Could not open file '%s'\n", ffiName.c_str() );
            return false;
        }
        
        serializer->Write( AssetType::ASSET_TYPE_SHADER );
        serializer->Write( inFile.getData(), inFile.size() );
        inFile.close();
    }

    return true;
}