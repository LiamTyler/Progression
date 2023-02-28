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

std::unordered_map<std::string, std::vector<std::string>> s_shaderIncludeCache;
static const char* SHADER_INCLUDE_PATH = PG_ASSET_DIR "cache/shader_includes.txt";
#define SHADER_INCLUDE_CACHE IN_USE

namespace PG
{

void InitShaderIncludeCache()
{
#if USING( SHADER_INCLUDE_CACHE )
    std::ifstream in( SHADER_INCLUDE_PATH );
    if ( !in )
        return;

    std::string line, shaderName;
    while ( std::getline( in, shaderName ) )
    {
        std::getline( in, line );
        uint32_t numIncludes = std::stoul( line );
        std::vector<std::string> includes( numIncludes );
        for ( uint32_t i = 0; i < numIncludes; ++i )
        {
            std::getline( in, line );
            includes[i] = PG_ASSET_DIR + line.substr( 1 ); // [0] char should be a tab
        }
        s_shaderIncludeCache[shaderName] = includes;
    }
#endif // #if USING( SHADER_INCLUDE_CACHE )
}

void CloseShaderIncludeCache()
{
#if USING( SHADER_INCLUDE_CACHE )
    std::ofstream out( SHADER_INCLUDE_PATH );
    if ( !out )
    {
        LOG_ERR( "Failed to save shader include cache to %s", SHADER_INCLUDE_PATH );
        return;
    }

    for ( const auto& [shaderName, includes] : s_shaderIncludeCache )
    {
        out << shaderName << std::endl;
        out << includes.size() << std::endl;
        for ( const auto& include : includes )
        {
            const std::string relPath = GetRelativePathToDir( include, PG_ASSET_DIR );
            out << "\t" << relPath << std::endl;
        }
    }
#endif // #if USING( SHADER_INCLUDE_CACHE )
}

std::string ShaderConverter::GetCacheNameInternal( ConstDerivedInfoPtr info )
{
    std::string cacheName = info->name;
    cacheName += "_" + GetRelativeFilename( info->filename );
    if ( info->generateDebugInfo )
    {
        cacheName += "_d";
    }
    return cacheName;
}


AssetStatus ShaderConverter::IsAssetOutOfDateInternal( ConstDerivedInfoPtr info, time_t cacheTimestamp )
{
#if USING( SHADER_INCLUDE_CACHE )
    std::string cacheName = GetCacheNameInternal( info );
    if ( s_shaderIncludeCache.contains( cacheName ) )
    {
        const auto& includeList = s_shaderIncludeCache[cacheName];
        for ( const std::string& includeFname : includeList )
        {
            AddFastfileDependency( includeFname );
            if ( IsFileOutOfDate( cacheTimestamp, includeFname ) )
                goto outOfDate;
        }

        return AssetStatus::UP_TO_DATE;
    }
outOfDate:
#endif // #if USING( SHADER_INCLUDE_CACHE )
    
    const ShaderPreprocessOutput preproc = PreprocessShader( *info, false );
    if ( !preproc.success )
    {
        LOG_ERR( "Preprocessing shader asset '%s' for the included files failed", info->name.c_str() );
        return AssetStatus::ERROR;
    }

#if USING( SHADER_INCLUDE_CACHE )
    if ( !s_shaderIncludeCache.contains( cacheName ) )
    {
        std::vector<std::string> dependentFiles;
        dependentFiles.reserve( 1 + preproc.includedFiles.size() );
        dependentFiles.push_back( info->filename );
        dependentFiles.insert( dependentFiles.end(), preproc.includedFiles.begin(), preproc.includedFiles.end() );

        s_shaderIncludeCache[cacheName] = std::move( dependentFiles );
    }
#endif // #if USING( SHADER_INCLUDE_CACHE )

    AddFastfileDependency( info->filename );
    bool outOfDate = IsFileOutOfDate( cacheTimestamp, info->filename );
    for ( const auto& file : preproc.includedFiles )
    {
        AddFastfileDependency( file );
        outOfDate = outOfDate || IsFileOutOfDate( cacheTimestamp, file );
    }

    if ( outOfDate )
    {
        s_cachedShaderPreproc[info->name] = std::move( preproc.outputShader );
        return AssetStatus::OUT_OF_DATE;
    }

    return AssetStatus::UP_TO_DATE;
}

} // namespace PG