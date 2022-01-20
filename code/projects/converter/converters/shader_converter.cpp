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


AssetStatus ShaderConverter::IsAssetOutOfDateInternal( ConstInfoPtr info, time_t cacheTimestamp )
{
    ShaderPreprocessOutput preproc = PreprocessShader( *info, false );
    if ( !preproc.success )
    {
        LOG_ERR( "Preprocessing shader asset '%s' for the included files failed", info->name.c_str() );
        return AssetStatus::ERROR;
    }

    AddFastfileDependency( info->filename );
    for ( const auto& file : preproc.includedFiles )
    {
        AddFastfileDependency( file );
    }

    bool outOfDate = IsFileOutOfDate( cacheTimestamp, info->filename ) || IsFileOutOfDate( cacheTimestamp, preproc.includedFiles );
    if ( outOfDate )
    {
        s_cachedShaderPreproc[info->name] = std::move( preproc.outputShader );
        return AssetStatus::OUT_OF_DATE;
    }

    return AssetStatus::UP_TO_DATE;
}

} // namespace PG