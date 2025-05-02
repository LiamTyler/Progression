#pragma once

#include "asset/asset_versions.hpp"
#include "asset/types/font.hpp"
#include "asset/types/gfx_image.hpp"
#include "asset/types/material.hpp"
#include "asset/types/model.hpp"
#include "asset/types/pipeline.hpp"
#include "asset/types/script.hpp"
#include "asset/types/shader.hpp"
#include "asset/types/ui_layout.hpp"
#include "shared/assert.hpp"
#include <unordered_map>

struct lua_State;

namespace PG::AssetManager
{

#if USING( CONVERTER ) || USING( OFFLINE_RENDERER )
extern std::unordered_map<std::string, BaseAsset*> g_resourceMaps[ASSET_TYPE_COUNT];
#endif // #if USING( CONVERTER ) || USING( OFFLINE_RENDERER )

struct GetAssetTypeIDHelper
{
    static u32 IDCounter;
};

template <typename Derived>
struct GetAssetTypeID : public GetAssetTypeIDHelper
{
    static u32 ID()
    {
        static u32 id = IDCounter++;
        return id;
    }
};

void Init();

void ProcessPendingLiveUpdates();

#if USING( ASSET_LIVE_UPDATE )
using AssetLiveUpdateList        = std::vector<std::pair<BaseAsset*, BaseAsset*>>;
using AssetLiveUpdateCallbackPtr = void ( * )( const AssetLiveUpdateList& assetPairs );
void AddLiveUpdateCallback( u32 assetTypeID, AssetLiveUpdateCallbackPtr callback );
#endif // #if USING( ASSET_LIVE_UPDATE )

bool LoadFastFile( const std::string& ffName, bool debugVersion = false );

void FreeRemainingGpuResources();
void Shutdown();

void RegisterLuaFunctions( lua_State* L );

BaseAsset* Get( u32 assetTypeID, const std::string& name );

template <typename T>
T* Get( const std::string& name )
{
    static_assert( std::is_base_of<BaseAsset, T>::value && !std::is_same<BaseAsset, T>::value,
        "Resource manager only manages classes derived from 'Resource'" );
    BaseAsset* asset;
#if USING( CONVERTER )
    u32 assetTypeID = GetAssetTypeID<T>::ID();
    PG_ASSERT( assetTypeID < ASSET_TYPE_COUNT, "Did you forget to update TOTAL_ASSET_TYPES?" );
    auto it = g_resourceMaps[assetTypeID].find( name );
    if ( it == g_resourceMaps[assetTypeID].end() )
    {
        // in the converter just want to get a list of asset names that are used while parsing the scene
        asset = new T;
        asset->SetName( name );
        g_resourceMaps[assetTypeID][name] = asset;
    }
    else
    {
        asset = it->second;
    }
#else  // #if USING( CONVERTER )
    asset = Get( GetAssetTypeID<T>::ID(), name );
#endif // #else // #if USING( CONVERTER )

    return static_cast<T*>( asset );
}

} // namespace PG::AssetManager
