#include "shader_converter.hpp"
#include "asset/shader_preprocessor.hpp"
#include "asset/types/shader.hpp"

namespace PG
{

static std::unordered_map< std::string, ShaderStage > shaderStageMap =
{
    { "vertex",                  ShaderStage::VERTEX },
    { "tessellation_control",    ShaderStage::TESSELLATION_CONTROL },
    { "tessellation_evaluation", ShaderStage::TESSELLATION_EVALUATION },
    { "geometry",                ShaderStage::GEOMETRY },
    { "fragment",                ShaderStage::FRAGMENT },
    { "compute",                 ShaderStage::COMPUTE },
};


void ShaderConverter::Parse( const rapidjson::Value& value )
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

    ShaderCreateInfo* info = new ShaderCreateInfo;
    info->shaderStage = ShaderStage::NUM_SHADER_STAGES;
    mapping.ForEachMember( value, *info );

    if ( !PathExists( info->filename ) )
    {
        LOG_ERR( "Shader file '%s' not found", info->filename.c_str() );
        g_converterStatus.parsingError = true;
    }

    if ( info->shaderStage == ShaderStage::NUM_SHADER_STAGES )
    {
        LOG_ERR( "Shader '%s' must have a valid ShaderStage!", info->name.c_str() );
        g_converterStatus.parsingError = true;
    }

    info->generateDebugInfo = g_converterConfigOptions.generateShaderDebugInfo;
    info->savePreproc = g_converterConfigOptions.saveShaderPreproc;

    m_parsedAssets.push_back( info );
}


std::string ShaderConverter::GetFastFileName( const BaseAssetCreateInfo* baseInfo ) const
{
    ShaderCreateInfo* info = (ShaderCreateInfo*)baseInfo;
    std::string baseName = info->name;
    baseName += "_" + GetRelativeFilename( info->filename );
    baseName += "_v" + std::to_string( PG_SHADER_VERSION );
    if ( info->generateDebugInfo )
    {
        baseName += "_d";
    }

    std::string fullName = PG_ASSET_DIR "cache/shaders/" + baseName + ".ffi";
    return fullName;
}


bool ShaderConverter::IsAssetOutOfDate( const BaseAssetCreateInfo* baseInfo )
{
    if ( g_converterConfigOptions.force )
    {
        return true;
    }

    ShaderCreateInfo* info = (ShaderCreateInfo*)baseInfo;
    bool savePreproc = info->savePreproc;
    info->savePreproc = false;
    std::string ffName = GetFastFileName( info );
    ShaderPreprocessOutput preproc = PreprocessShader( *info );
    info->savePreproc = savePreproc;
    if ( !preproc.success )
    {
        LOG_ERR( "Preprocessing shader asset '%s' for the included files failed", info->name.c_str() );
        g_converterStatus.checkDependencyErrors += 1;
        return false;
    }
    info->preprocOutput = std::move( preproc.outputShader );

    AddFastfileDependency( ffName );
    for ( const auto& file : preproc.includedFiles )
    {
        AddFastfileDependency( file );
    }

    return IsFileOutOfDate( ffName, info->filename ) || IsFileOutOfDate( ffName, preproc.includedFiles );
}


bool ShaderConverter::ConvertSingle( const BaseAssetCreateInfo* baseInfo ) const
{
    ShaderCreateInfo* info = (ShaderCreateInfo*)baseInfo;
    LOG( "Converting Shader file '%s'...", info->filename.c_str() );
    Shader asset;
    if ( !Shader_Load( &asset, *info ) )
    {
        return false;
    }
    std::string fastfileName = GetFastFileName( info );
    Serializer serializer;
    if ( !serializer.OpenForWrite( fastfileName ) )
    {
        return false;
    }
    if ( !Fastfile_Shader_Save( &asset, &serializer ) )
    {
        LOG_ERR( "Error while writing Shader '%s' to fastfile", info->name.c_str() );
        serializer.Close();
        DeleteFile( fastfileName );
        return false;
    }
    
    serializer.Close();

    return true;
}


} // namespace PG