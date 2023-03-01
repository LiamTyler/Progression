#include "shader_converter.hpp"
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

void AddIncludeCacheEntry( const std::string& cacheName, const ShaderCreateInfo* createInfo, const ShaderPreprocessOutput& preprocOutput )
{
#if USING( SHADER_INCLUDE_CACHE )
    std::vector<std::string> dependentFiles;
    dependentFiles.reserve( 1 + preprocOutput.includedFilesAbsPath.size() );
    dependentFiles.push_back( GetAbsPath_ShaderFilename( createInfo->filename ) );
    for ( const auto& file : preprocOutput.includedFilesAbsPath )
        dependentFiles.push_back( file );

    if ( !s_shaderIncludeCache.contains( cacheName ) )
        s_shaderIncludeCache[cacheName] = std::move( dependentFiles );
    else
        PG_ASSERT( s_shaderIncludeCache[cacheName] == dependentFiles )
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
                return AssetStatus::OUT_OF_DATE;
        }

        return AssetStatus::UP_TO_DATE;
    }
#endif // #if USING( SHADER_INCLUDE_CACHE )

//#if USING( SHADER_INCLUDE_CACHE )
//    if ( !s_shaderIncludeCache.contains( cacheName ) )
//    {
//        std::vector<std::string> dependentFiles;
//        dependentFiles.reserve( 1 + preproc.includedFilesAbsPath.size() );
//        dependentFiles.push_back( GetAbsPath_ShaderFilename( info->filename ) );
//        for ( const auto& file : preproc.includedFilesAbsPath )
//            dependentFiles.push_back( file );
//
//        s_shaderIncludeCache[cacheName] = std::move( dependentFiles );
//    }
//#endif // #if USING( SHADER_INCLUDE_CACHE )

    return AssetStatus::OUT_OF_DATE;

}

} // namespace PG