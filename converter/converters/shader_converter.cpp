#include "converter.hpp"
#include "asset/shader_preprocessor.hpp"
#include "asset/types/shader.hpp"

using namespace PG;

extern void AddFastfileDependency( const std::string& file );


static std::vector< ShaderCreateInfo > s_parsedShaders;
static std::vector< ShaderCreateInfo > s_outOfDateShaders;

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
        { "name",        []( const rapidjson::Value& v, ShaderCreateInfo& s ) { s.name     = v.GetString(); } },
        { "filename",    []( const rapidjson::Value& v, ShaderCreateInfo& s ) { s.filename = PG_ASSET_DIR "shaders/" + std::string( v.GetString() ); } },
        { "shaderStage", []( const rapidjson::Value& v, ShaderCreateInfo& s )
            {
                std::string stageName = v.GetString();
                auto it = shaderStageMap.find( stageName );
                if ( it == shaderStageMap.end() )
                {
                    LOG_ERR( "No ShaderStage found matching '%s'", stageName.c_str() );
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
        LOG_ERR( "Shader file '%s' not found", info.filename.c_str() );
        g_converterStatus.parsingError = true;
    }

    if ( info.shaderStage == ShaderStage::NUM_SHADER_STAGES )
    {
        LOG_ERR( "Shader '%s' must have a valid ShaderStage!", info.name.c_str() );
        g_converterStatus.parsingError = true;
    }

    info.generateDebugInfo = g_converterConfigOptions.generateShaderDebugInfo;
    info.savePreproc = g_converterConfigOptions.saveShaderPreproc;

    s_parsedShaders.push_back( info );
}


static std::string Shader_GetFastFileName( const ShaderCreateInfo& info )
{
    std::string baseName = info.name;
    baseName += "_" + GetRelativeFilename( info.filename );
    baseName += "_v" + std::to_string( PG_SHADER_VERSION );
    if ( info.generateDebugInfo )
    {
        baseName += "_d";
    }

    std::string fullName = PG_ASSET_DIR "cache/shaders/" + baseName + ".ffi";
    return fullName;
}


static bool Shader_IsOutOfDate( ShaderCreateInfo& info )
{
    if ( g_converterConfigOptions.force )
    {
        return true;
    }
    bool savePreproc = info.savePreproc;
    info.savePreproc = false;
    std::string ffName = Shader_GetFastFileName( info );
    ShaderPreprocessOutput preproc = PreprocessShader( info );
    info.savePreproc = savePreproc;
    if ( !preproc.success )
    {
        LOG_ERR( "Preprocessing shader asset '%s' for the included files failed", info.name.c_str() );
        g_converterStatus.checkDependencyErrors += 1;
        return false;
    }
    info.preprocOutput = std::move( preproc.outputShader );

    AddFastfileDependency( ffName );
    for ( const auto& file : preproc.includedFiles )
    {
        AddFastfileDependency( file );
    }

    return IsFileOutOfDate( ffName, info.filename ) || IsFileOutOfDate( ffName, preproc.includedFiles );
}


static bool Shader_ConvertSingle( const ShaderCreateInfo& info )
{
    LOG( "Converting Shader file '%s'...", info.filename.c_str() );
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
        LOG_ERR( "Error while writing Shader '%s' to fastfile", info.name.c_str() );
        serializer.Close();
        DeleteFile( fastfileName );
        return false;
    }
    
    serializer.Close();

    return true;
}


int Shader_Convert()
{
    if ( s_outOfDateShaders.size() == 0 )
    {
        return 0;
    }

    int couldNotConvert = 0;
    for ( int i = 0; i < (int)s_outOfDateShaders.size(); ++i )
    {
        if ( !Shader_ConvertSingle( s_outOfDateShaders[i] ) )
        {
            ++couldNotConvert;
        }
    }

    return couldNotConvert;
}


int Shader_CheckDependencies()
{
    int outOfDate = 0;
    for ( size_t i = 0; i < s_parsedShaders.size(); ++i )
    {
        if ( Shader_IsOutOfDate( s_parsedShaders[i] ) )
        {
            s_outOfDateShaders.push_back( s_parsedShaders[i] );
            ++outOfDate;
        }
    }

    return outOfDate;
}


bool Shader_BuildFastFile( Serializer* serializer )
{
    for ( size_t i = 0; i < s_parsedShaders.size(); ++i )
    {
        std::string ffiName = Shader_GetFastFileName( s_parsedShaders[i] );
        MemoryMapped inFile;
        if ( !inFile.open( ffiName ) )
        {
            LOG_ERR( "Could not open file '%s'", ffiName.c_str() );
            return false;
        }
        
        serializer->Write( AssetType::ASSET_TYPE_SHADER );
        serializer->Write( inFile.getData(), inFile.size() );
        inFile.close();
    }

    return true;
}