#include "shader_converter.hpp"
#include "asset/shader_preprocessor.hpp"
#include "shared/hash.hpp"
#include <map>

std::map<std::string, std::vector<std::string>> s_shaderIncludeCache;
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
            includes[i] = PG_ASSET_DIR "shaders/" + line.substr( 1 ); // [0] char should be a tab
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
        for ( const auto& absPath : includes )
        {
            std::string relPath = GetRelativePathToDir( absPath, PG_ASSET_DIR "shaders/" );
            out << "\t" << relPath << std::endl;
        }
    }
#endif // #if USING( SHADER_INCLUDE_CACHE )
}

std::string ShaderConverter::GetCacheNameInternal( ConstDerivedInfoPtr info )
{
    std::string cacheName = info->name;
    cacheName += "_" + std::to_string( Hash( info->filename ) );
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
                goto cacheMiss;
        }

        return AssetStatus::UP_TO_DATE;
    }
cacheMiss:
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
        dependentFiles.reserve( 1 + preproc.includedFilesAbsPath.size() );
        dependentFiles.push_back( GetAbsPath_ShaderFilename( info->filename ) );
        for ( const auto& file : preproc.includedFilesAbsPath )
            dependentFiles.push_back( file );

        s_shaderIncludeCache[cacheName] = std::move( dependentFiles );
    }
#endif // #if USING( SHADER_INCLUDE_CACHE )

    const std::string absPathFilename = GetAbsPath_ShaderFilename( info->filename );
    AddFastfileDependency( absPathFilename );
    bool outOfDate = IsFileOutOfDate( cacheTimestamp, absPathFilename );
    for ( const auto& file : preproc.includedFilesAbsPath )
    {
        AddFastfileDependency( file );
        outOfDate = outOfDate || IsFileOutOfDate( cacheTimestamp, file );
    }

    return outOfDate ? AssetStatus::OUT_OF_DATE : AssetStatus::UP_TO_DATE;
}

} // namespace PG