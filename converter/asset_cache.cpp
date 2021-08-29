#include "asset_cache.hpp"
#include "utils/filesystem.hpp"
#include "core/platform_defines.hpp"
#include "utils/file_dependency.hpp"
#include "utils/serializer.hpp"

#define ROOT_DIR std::string( PG_ASSET_DIR "cache/" )

std::string assetCacheFolders[NUM_ASSET_TYPES] =
{
    "images/",    // ASSET_TYPE_GFX_IMAGE
    "materials/", // ASSET_TYPE_MATERIAL
    "scripts/",   // ASSET_TYPE_SCRIPT
    "models/",    // ASSET_TYPE_MODEL
    "shaders/",   // ASSET_TYPE_SHADER
};

static std::string GetCachedPath( AssetType assetType, const std::string& assetCacheName )
{
    return ROOT_DIR + assetCacheFolders[assetType] + assetCacheName + ".ffi";
}

namespace PG::AssetCache
{

void Init()
{
    CreateDirectory( ROOT_DIR );
    for ( int i = 0; i < NUM_ASSET_TYPES; ++i )
    {
        CreateDirectory( ROOT_DIR + assetCacheFolders[i] );
    }
    CreateDirectory( ROOT_DIR + "fastfiles/" );
    CreateDirectory( ROOT_DIR + "shader_preproc/" );
}


time_t GetAssetTimestamp( AssetType assetType, const std::string& assetCacheName )
{
    return GetFileTimestamp( GetCachedPath( assetType, assetCacheName ) );
}


bool CacheAsset( AssetType assetType, const std::string& assetCacheName, BaseAsset* asset )
{
    std::string path = GetCachedPath( assetType, assetCacheName );
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


std::unique_ptr<char[]> GetCachedAssetRaw( AssetType assetType, const std::string& assetCacheName, size_t& numBytes )
{
    numBytes = 0;
    std::string path = GetCachedPath( assetType, assetCacheName );
    Serializer in;
    if ( !in.OpenForRead( path ) )
    {
        return false;
    }
    numBytes = in.BytesLeft();
    std::unique_ptr<char[]> ret( new char[numBytes] );
    in.Read( ret.get(), numBytes );
    return ret;
}

} // namespace PG::AssetCache