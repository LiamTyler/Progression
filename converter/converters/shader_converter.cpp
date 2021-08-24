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


std::shared_ptr<BaseAssetCreateInfo> ShaderConverter::Parse( const rapidjson::Value& value, std::shared_ptr<const BaseAssetCreateInfo> parent )
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

    auto info = std::make_shared<ShaderCreateInfo>();
    if ( parent )
    {
        *info = *std::static_pointer_cast<const ShaderCreateInfo>( parent );
    }
    mapping.ForEachMember( value, *info );

    if ( info->shaderStage == ShaderStage::NUM_SHADER_STAGES ) PARSE_ERROR( "Shader '%s' must have a valid ShaderStage!", info->name.c_str() );

    return info;
}


std::string ShaderConverter::GetFastFileName( const BaseAssetCreateInfo* baseInfo ) const
{
    const ShaderCreateInfo* info = (const ShaderCreateInfo*)baseInfo;
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

    std::string ffName = GetFastFileName( baseInfo );

    // remove const qualifier so we can set savePreproc and return it later, to avoid making copy instead
    ShaderCreateInfo* info = (ShaderCreateInfo*)baseInfo;
    bool savePreproc = info->savePreproc;
    info->savePreproc = false;
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
    return ConvertSingleInternal< Shader, ShaderCreateInfo >( baseInfo );
}


} // namespace PG