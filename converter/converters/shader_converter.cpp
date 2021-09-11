#include "shader_converter.hpp"
#include "asset/shader_preprocessor.hpp"

static std::unordered_map<std::string, std::string> s_cachedShaderPreproc;

const std::string* GetShaderPreproc( const std::string& shaderName )
{
    auto it = s_cachedShaderPreproc.find( shaderName );
    if ( it == s_cachedShaderPreproc.end() )
    {
        return nullptr;
    }
    return &it->second;
}


void ReleaseShaderPreproc( const std::string& shaderName )
{
    s_cachedShaderPreproc.erase( shaderName );
}

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


ConvertDate ShaderConverter::IsAssetOutOfDateInternal( ConstInfoPtr info, time_t cacheTimestamp )
{
    ShaderPreprocessOutput preproc = PreprocessShader( *info, false );
    if ( !preproc.success )
    {
        LOG_ERR( "Preprocessing shader asset '%s' for the included files failed", info->name.c_str() );
        return ConvertDate::ERROR;
    }

    for ( const auto& file : preproc.includedFiles )
    {
        AddFastfileDependency( file );
    }

    bool outOfDate = IsFileOutOfDate( cacheTimestamp, info->filename ) || IsFileOutOfDate( cacheTimestamp, preproc.includedFiles );
    if ( outOfDate )
    {
        s_cachedShaderPreproc[info->name] = std::move( preproc.outputShader );
        return ConvertDate::OUT_OF_DATE;
    }

    return ConvertDate::UP_TO_DATE;
}

} // namespace PG