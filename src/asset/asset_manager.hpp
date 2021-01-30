#pragma once

#include "asset/asset_versions.hpp"
#include "asset/types/base_asset.hpp"
#include "asset/types/gfx_image.hpp"
#include "asset/types/material.hpp"
#include "asset/types/model.hpp"
#include "asset/types/script.hpp"
#include "asset/types/shader.hpp"

namespace PG
{
namespace AssetManager
{

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

Asset* GetInternal( uint32_t assetTypeID, const std::string& name );

template < typename T >
T* Get( const std::string& name )
{
    static_assert( std::is_base_of< Asset, T >::value && !std::is_same< Asset, T >::value, "Resource manager only manages classes derived from 'Resource'" );
    Asset* res = GetInternal( GetAssetTypeID< T >::ID(), name );
    return static_cast< T* >( res );
}


void AddInternal( uint32_t assetTypeID, Asset* asset );

// asset must be allowed with "new"
template < typename T >
void Add( T* asset )
{
    static_assert( std::is_base_of< Asset, T >::value && !std::is_same< Asset, T >::value, "Resource manager only manages classes derived from 'Resource'" );
    PG_ASSERT( asset );
    AddInternal( GetAssetTypeID< T >::ID(), static_cast< Asset* >( asset ) );
}


} // namespace AssetManager
} // namesapce PG