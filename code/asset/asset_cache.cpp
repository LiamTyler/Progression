#include "asset_cache.hpp"
#include "core/feature_defines.hpp"
#include "shared/file_dependency.hpp"
#include "shared/filesystem.hpp"
#include "shared/serializer.hpp"
#include "xxHash/xxhash.h"

#define ROOT_DIR std::string( PG_ASSET_DIR "cache/" )

std::string assetCacheFolders[ASSET_TYPE_COUNT] = {
    "images/",      // ASSET_TYPE_GFX_IMAGE
    "materials/",   // ASSET_TYPE_MATERIAL
    "scripts/",     // ASSET_TYPE_SCRIPT
    "models/",      // ASSET_TYPE_MODEL
    "shaders/",     // ASSET_TYPE_SHADER
    "ui_layouts/",  // ASSET_TYPE_UI_LAYOUT
    "pipelines/",   // ASSET_TYPE_PIPELINE
    "fonts/",       // ASSET_TYPE_FONT
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
    ".ffi", // ASSET_TYPE_FONT
    ".ffi", // ASSET_TYPE_TEXTURESET
};
static_assert( ASSET_TYPE_COUNT == 9 );

static std::string GetCachedPath( AssetType assetType, const std::string& assetCacheName )
{
    return ROOT_DIR + assetCacheFolders[assetType] + assetCacheName + assetCacheFileExtensions[assetType];
}

namespace PG::AssetCache
{

void Init()
{
    PGP_ZONE_SCOPEDN( "AssetCache::Init" );
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

struct HashData
{
    XXH64_state_t* state;
    XXH64_hash_t seed;

    HashData()
    {
        state = XXH64_createState();
        seed  = 0;
        XXH64_reset( state, seed );
    }

    size_t Finalize()
    {
        static_assert( sizeof( XXH64_hash_t ) == sizeof( size_t ) );
        XXH64_hash_t const hash = XXH64_digest( state );
        XXH64_freeState( state );
        return hash;
    }
};

void SerializeCallback( const void* buffer, size_t size, void* userdata )
{
    HashData* hashData = (HashData*)userdata;
    XXH64_update( hashData->state, buffer, size );
}

bool CacheAsset( AssetType assetType, const std::string& assetCacheName, BaseAsset* asset )
{
    PGP_ZONE_SCOPEDN( "CacheAsset" );
    AssetMetadata metadata = {};
    metadata.name          = asset->GetName();

    std::string tmpPath   = GetCachedPath( assetType, assetCacheName + "_tmp" );
    std::string finalPath = GetCachedPath( assetType, assetCacheName );
    try
    {
        HashData hashData{};
        Serializer serializer;
        serializer.SetOnFlushCallback( SerializeCallback, &hashData );
        if ( !serializer.OpenForWrite( tmpPath, true ) )
        {
            return false;
        }
        if ( !asset->FastfileSave( &serializer ) )
        {
            serializer.Close();
            DeleteFile( tmpPath );
            return false;
        }

        // Fast path, for when the entire asset fits into the scratch buffer (very often)
        // this way we avoid writing it to disk, and then reading it immediately again
        // Close to 7x fater, on my windows machine at leat
        if ( !serializer.HasFlushed() )
        {
            serializer.RunFlushCallback();
            serializer.ChangeFilename( finalPath );
            if ( !serializer.FinalizeOpenWriteFile() )
                return false;

            metadata.size = serializer.BytesWritten();
            metadata.hash = hashData.Finalize();

            // keep in sync with SerializeAssetMetadata
            u16 nameLen = static_cast<u16>( metadata.name.length() );
            serializer.GetWriteFile().write( (char*)&nameLen, sizeof( u16 ) );
            serializer.GetWriteFile().write( metadata.name.c_str(), nameLen );
            serializer.GetWriteFile().write( (char*)&metadata.hash, sizeof( u64 ) );
            serializer.GetWriteFile().write( (char*)&metadata.size, sizeof( u64 ) );
            serializer.Close();
            return true;
        }

        metadata.size = serializer.Close();
        metadata.hash = hashData.Finalize();
    }
    catch ( std::exception& e )
    {
        DeleteFile( tmpPath );
        throw e;
    }

    try
    {
        Serializer inputSerializer;
        if ( !inputSerializer.OpenForRead( tmpPath ) )
            return false;

        Serializer outSerializer;
        if ( !outSerializer.OpenForWrite( finalPath ) )
            return false;

        SerializeAssetMetadata( &outSerializer, metadata );
        outSerializer.Write( inputSerializer.GetData(), inputSerializer.BytesLeft() );
    }
    catch ( std::exception& e )
    {
        DeleteFile( tmpPath );
        DeleteFile( finalPath );
        throw e;
    }

    DeleteFile( tmpPath );

    return true;
}

std::unique_ptr<char[]> GetCachedAssetRaw( AssetType assetType, const std::string& assetCacheName, size_t& numBytes )
{
    PGP_ZONE_SCOPEDN( "GetCachedAssetRaw" );
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
