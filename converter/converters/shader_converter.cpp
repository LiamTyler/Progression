#include "shader_converter.hpp"
#include "asset/shader_preprocessor.hpp"

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

bool ShaderConverter::ParseInternal( const rapidjson::Value& value, InfoPtr info )
{
    static JSONFunctionMapper<ShaderCreateInfo&> mapping(
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
    mapping.ForEachMember( value, *info );

    if ( info->shaderStage == ShaderStage::NUM_SHADER_STAGES ) PARSE_ERROR( "Shader '%s' must have a valid ShaderStage!", info->name.c_str() );
    return true;
}



std::string ShaderConverter::GetCacheNameInternal( ConstInfoPtr info )
{
    std::string cacheName = info->name;
    cacheName += "_" + GetRelativeFilename( info->filename );
    if ( info->generateDebugInfo )
    {
        cacheName += "_d";
    }
    return cacheName;
}


bool ShaderConverter::IsAssetOutOfDateInternal( ConstInfoPtr info, time_t cacheTimestamp )
{
    ShaderPreprocessOutput preproc = PreprocessShader( *info, false );
    if ( !preproc.success )
    {
        LOG_ERR( "Preprocessing shader asset '%s' for the included files failed", info->name.c_str() );
        g_converterStatus.error = true;
        return true;
    }
    // std::move( preproc.outputShader ); todo: cache shader preproc for compiling later

    for ( const auto& file : preproc.includedFiles )
    {
        AddFastfileDependency( file );
    }

    return IsFileOutOfDate( cacheTimestamp, info->filename ) || IsFileOutOfDate( cacheTimestamp, preproc.includedFiles );
}

} // namespace PG