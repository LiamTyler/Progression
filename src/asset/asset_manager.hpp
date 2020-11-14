#pragma once

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

Asset* Get( uint32_t assetTypeID, const std::string& name );

template < typename T >
T* Get( const std::string& name )
{
    static_assert( std::is_base_of< Asset, T >::value && !std::is_same< Asset, T >::value, "Resource manager only manages classes derived from 'Resource'" );
    Asset* res = Get( GetAssetTypeID< T >::ID(), name );
    return static_cast< T* >( res );
}


} // namespace AssetManager
} // namesapce PG