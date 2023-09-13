#include "shader_converter.hpp"
#include "shared/hash.hpp"

#define SHADER_INCLUDE_CACHE IN_USE

#if USING( SHADER_INCLUDE_CACHE )
#include <map>
#include <mutex>

std::map<std::string, std::vector<std::string>> s_shaderIncludeCache;
static std::mutex s_shaderIncludeCacheLock;
static const char* SHADER_INCLUDE_PATH = PG_ASSET_DIR "cache/shader_includes.txt";

void PG::InitShaderIncludeCache()
{
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
}

void PG::CloseShaderIncludeCache()
{
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
}

void PG::AddIncludeCacheEntry( const std::string& cacheName, const ShaderCreateInfo* createInfo, const ShaderPreprocessOutput& preprocOutput )
{
    std::vector<std::string> dependentFiles;
    dependentFiles.reserve( 1 + preprocOutput.includedFilesAbsPath.size() );
    dependentFiles.push_back( GetAbsPath_ShaderFilename( createInfo->filename ) );
    for ( const auto& file : preprocOutput.includedFilesAbsPath )
        dependentFiles.push_back( file );

    std::scoped_lock lock( s_shaderIncludeCacheLock );
    s_shaderIncludeCache[cacheName] = std::move( dependentFiles );
}

static bool GetIncludeCacheEntry( const std::string& cacheName, std::vector<std::string>& entry )
{
    std::scoped_lock lock( s_shaderIncludeCacheLock );
    if ( s_shaderIncludeCache.contains( cacheName ) )
    {
        entry = s_shaderIncludeCache[cacheName];
        return true;
    }

    return false;
}

#else // #if USING( SHADER_INCLUDE_CACHE )
void PG::InitShaderIncludeCache() {}
void PG::CloseShaderIncludeCache() {}
void PG::AddIncludeCacheEntry( const std::string& cacheName, const ShaderCreateInfo* createInfo, const ShaderPreprocessOutput& preprocOutput )
{
    PG_UNUSED( cacheName ); PG_UNUSED( createInfo ); PG_UNUSED( preprocOutput );
}
#endif // #else // #if USING( SHADER_INCLUDE_CACHE )


namespace PG
{

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
    std::vector<std::string> includeList;
    if ( GetIncludeCacheEntry( cacheName, includeList ) )
    {
        for ( const std::string& includeFname : includeList )
        {
            AddFastfileDependency( includeFname );
            if ( IsFileOutOfDate( cacheTimestamp, includeFname ) )
                return AssetStatus::OUT_OF_DATE;
        }

        return AssetStatus::UP_TO_DATE;
    }
#endif // #if USING( SHADER_INCLUDE_CACHE )

    // Generating the preproc is relatively expensive, compared to compiling it to spirv actually (currently)
    // so instead of generating it here, and testing each file for out of out of date and calling
    // AddFastfileDependency(), just reconvert the asset (which automatically regenerates the fastfile)
    return AssetStatus::OUT_OF_DATE;
}

} // namespace PG