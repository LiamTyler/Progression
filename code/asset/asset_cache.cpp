#include "asset_cache.hpp"
#include "core/feature_defines.hpp"
#include "shared/file_dependency.hpp"
#include "shared/filesystem.hpp"
#include "shared/serializer.hpp"

#define ROOT_DIR std::string( PG_ASSET_DIR "cache/" )

std::string assetCacheFolders[ASSET_TYPE_COUNT] = {
    "images/",      // ASSET_TYPE_GFX_IMAGE
    "materials/",   // ASSET_TYPE_MATERIAL
    "scripts/",     // ASSET_TYPE_SCRIPT
    "models/",      // ASSET_TYPE_MODEL
    "shaders/",     // ASSET_TYPE_SHADER
    "ui_layouts/",  // ASSET_TYPE_UI_LAYOUT
    "pipelines/",   // ASSET_TYPE_PIPELINE
    "texturesets/", // ASSET_TYPE_TEXTURESET
};

std::string assetCacheFileExtensions[ASSET_TYPE_COUNT] = {
    ".pgi", // ASSET_TYPE_GFX_IMAGE
    ".ffi", // ASSET_TYPE_MATERIAL
    ".ffi", // ASSET_TYPE_SCRIPT
    ".ffi", // ASSET_TYPE_MODEL
    ".ffi", // ASSET_TYPE_SHADER
    ".ffi", // ASSET_TYPE_UI_LAYOUT
    ".ffi", // ASSET_TYPE_PIPELINE
    ".ffi", // ASSET_TYPE_TEXTURESET
};
static_assert( ASSET_TYPE_COUNT == 8 );

static std::string GetCachedPath( AssetType assetType, const std::string& assetCacheName )
{
    return ROOT_DIR + assetCacheFolders[assetType] + assetCacheName + assetCacheFileExtensions[assetType];
}

namespace PG::AssetCache
{

void Init()
{
    CreateDirectory( ROOT_DIR );
    for ( u32 i = 0; i < ASSET_TYPE_COUNT; ++i )
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
    numBytes         = 0;
    std::string path = GetCachedPath( assetType, assetCacheName );
    Serializer in;
    if ( !in.OpenForRead( path ) )
    {
        return nullptr;
    }
    numBytes = in.BytesLeft();
    std::unique_ptr<char[]> ret( new char[numBytes] );
    in.Read( ret.get(), numBytes );
    return ret;
}

} // namespace PG::AssetCache
