#pragma once

#include "asset/types/base_asset.hpp"
#include "asset/types/gfx_image.hpp"
#include "asset/types/material.hpp"
#include "asset/types/model.hpp"
#include "asset/types/script.hpp"
#include "asset/types/shader.hpp"
#include "asset/asset_versions.hpp"
#include <unordered_map>

struct lua_State;

namespace PG::AssetManager
{

#if USING( CONVERTER ) || USING( OFFLINE_RENDERER )
    extern std::unordered_map< std::string, BaseAsset* > g_resourceMaps[AssetType::ASSET_TYPE_COUNT];
#endif // #if USING( CONVERTER ) || USING( OFFLINE_RENDERER )

struct GetAssetTypeIDHelper
{
    static uint32_t IDCounter;
};

template <typename Derived>
struct GetAssetTypeID : public GetAssetTypeIDHelper
{
    static uint32_t ID()
    {
        static uint32_t id = IDCounter++;
        return id;
    }
};


void Init();

void ProcessPendingLiveUpdates();

bool LoadFastFile( const std::string& fname );

void Shutdown();

void RegisterLuaFunctions( lua_State* L );

BaseAsset* Get( uint32_t assetTypeID, const std::string& name );

template < typename T >
T* Get( const std::string& name )
{
    static_assert( std::is_base_of<BaseAsset, T>::value && !std::is_same<BaseAsset, T>::value, "Resource manager only manages classes derived from 'Resource'" );
    BaseAsset* asset;
#if USING( CONVERTER )
    uint32_t assetTypeID = GetAssetTypeID<T>::ID();
    PG_ASSERT( assetTypeID < AssetType::ASSET_TYPE_COUNT, "Did you forget to update TOTAL_ASSET_TYPES?" );
    auto it = g_resourceMaps[assetTypeID].find( name );
    if ( it == g_resourceMaps[assetTypeID].end() )
    {
        // in the converter just want to get a list of asset names that are used while parsing the scene
        asset = new T;
        asset->name = name;
        g_resourceMaps[assetTypeID][name] = asset;
    }
    else
    {
        asset = it->second;
    }
#else // #if USING( CONVERTER )
    asset = Get( GetAssetTypeID<T>::ID(), name );
#endif // #else // #if USING( CONVERTER )

    return static_cast<T*>( asset );
}

} // namesapce PG::AssetManager