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

#if USING( COMPILING_CONVERTER )
    extern std::unordered_map< std::string, BaseAsset* > g_resourceMaps[AssetType::NUM_ASSET_TYPES];
#endif // #if USING( COMPILING_CONVERTER )

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

bool LoadFastFile( const std::string& fname );

void Shutdown();

void RegisterLuaFunctions( lua_State* L );

BaseAsset* Get( uint32_t assetTypeID, const std::string& name );

template < typename T >
T* Get( const std::string& name )
{
    static_assert( std::is_base_of<BaseAsset, T>::value && !std::is_same<BaseAsset, T>::value, "Resource manager only manages classes derived from 'Resource'" );

    // in the converter just want to get a list of asset names that are used while parsing the scene
#if USING( COMPILING_CONVERTER )
    auto asset = Get( GetAssetTypeID<T>::ID(), name );
    if ( !asset )
    {
        auto newAsset = new T;
        newAsset->name = name;
        g_resourceMaps[GetAssetTypeID<T>::ID()][name] = newAsset;
        return newAsset;
    }
    else
    {
        return static_cast<T*>( asset );
    }
#else // #if USING( COMPILING_CONVERTER )
    BaseAsset* asset = Get( GetAssetTypeID<T>::ID(), name );
    return static_cast<T*>( asset );
#endif // #else // #if USING( COMPILING_CONVERTER )
}

} // namesapce PG::AssetManager