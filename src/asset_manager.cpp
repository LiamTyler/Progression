#include "asset_manager.hpp"
#include "assert.hpp"
#include "asset_versions.hpp"
#include "gfx_image.hpp"
#include "material.hpp"
#include "utils/logger.hpp"
#include "utils/serializer.hpp"
#include <unordered_map>


namespace Progression
{
namespace AssetManager
{

uint32_t GetAssetTypeIDHelper::IDCounter = 0;



static std::unordered_map< std::string, Asset* > s_resourceMaps[AssetType::NUM_ASSET_TYPES];


void Init()
{
    GetAssetTypeID< GfxImage >::ID(); // AssetType::ASSET_TYPE_GFX_IMAGE
    GetAssetTypeID< Material >::ID(); // AssetType::ASSET_TYPE_MATERIAL
    static_assert( NUM_ASSET_TYPES == 2, "Dont forget to add GetAssetTypeID for new assets" );
}

#define APPEND( x ) Fastfile_##x##_Load
#define ASSET_LOAD( type, enumID ) \
case enumID: \
{ \
    type* asset = new type; \
    if ( !APPEND( type )( asset, &serializer ) ) \
    { \
        LOG_ERR( "Could not load GfxImage\n" ); \
        return false; \
    } \
    auto it = s_resourceMaps[enumID].find( asset->name ); \
    if ( it == s_resourceMaps[enumID].end() ) \
    { \
        s_resourceMaps[enumID][asset->name] = asset; \
    } \
    else \
    { \
        s_resourceMaps[enumID][asset->name]->Move( asset ); \
    } \
    break; \
}

bool LoadFastFile( const std::string& fname )
{
    std::string fullName = PG_ASSET_DIR "cache/fastfiles/" + fname + "_v" + std::to_string( PG_FASTFILE_VERSION ) + ".ff";
    Serializer serializer;
    if ( !serializer.OpenForRead( fullName ) )
    {
        return false;
    }

    while ( serializer.BytesLeft() > 0 )
    {
        AssetType assetType;
        serializer.Read( assetType );
        PG_ASSERT( assetType < AssetType::NUM_ASSET_TYPES );
        switch ( assetType )
        {
        ASSET_LOAD( GfxImage, ASSET_TYPE_GFX_IMAGE );
        ASSET_LOAD( Material, ASSET_TYPE_MATERIAL );
        default:
            LOG_ERR( "Unknown asset type '%d'\n", (int)assetType );
            return false;
        }
    }

    return true;
}


void Shutdown()
{
    for ( int i = 0; i < AssetType::NUM_ASSET_TYPES; ++i )
    {
        for ( const auto& it : s_resourceMaps[i] )
        {
            delete it.second;
        }
        s_resourceMaps[i].clear();
    }
}


Asset* Get( uint32_t assetTypeID, const std::string& name )
{
    PG_ASSERT( assetTypeID < AssetType::NUM_ASSET_TYPES, "Did you forget to update TOTAL_ASSET_TYPES?" );
    auto it = s_resourceMaps[assetTypeID].find( name );
    if ( it != s_resourceMaps[assetTypeID].end() )
    {
        return it->second;
    }

    return nullptr;
}


} // namespace AssetManager
} // namesapce Progression