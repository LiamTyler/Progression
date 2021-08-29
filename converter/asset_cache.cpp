#include "asset_cache.hpp"
#include "utils/filesystem.hpp"
#include "core/platform_defines.hpp"
#include "utils/file_dependency.hpp"
#include "utils/serializer.hpp"

#define ROOT_DIR PG_ASSET_DIR "cache/"

std::string assetCacheFolders[NUM_ASSET_TYPES] =
{
    "images/",    // ASSET_TYPE_GFX_IMAGE
    "materials/", // ASSET_TYPE_MATERIAL
    "scripts/",   // ASSET_TYPE_SCRIPT
    "models/",    // ASSET_TYPE_MODEL
    "shaders/",   // ASSET_TYPE_SHADER
};

namespace PG::AssetCache
{

void Init()
{
    CreateDirectory( ROOT_DIR );
    for ( int i = 0; i < NUM_ASSET_TYPES; ++i )
    {
        CreateDirectory( ROOT_DIR + assetCacheFolders[i] );
    }
    CreateDirectory( ROOT_DIR "fastfiles/" );
    CreateDirectory( ROOT_DIR "shader_preproc/" );
}


time_t GetAssetTimestamp( AssetType assetType, const std::string& assetCacheName )
{
    std::string path = ROOT_DIR + assetCacheFolders[assetType] + assetCacheName + ".ffi";
    return GetFileTimestamp( path );
}


bool CacheAsset( AssetType assetType, const std::string& assetCacheName, BaseAsset* asset )
{
    std::string path = ROOT_DIR + assetCacheFolders[assetType] + assetCacheName + ".ffi";

    try
    {
        Serializer serializer;
        if ( !serializer.OpenForWrite( path ) )
        {
            return false;
        }
        if ( !asset->FastfileSave( &serializer ) )
        {
            serializer.Close();
            DeleteFile( path );
            return false;
        }
        serializer.Close();
    }
    catch ( std::exception& e )
    {
        DeleteFile( path );
        throw e;
    }

    return true;
}

} // namespace PG::AssetCache