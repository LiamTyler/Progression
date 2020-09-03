#include "asset_manager.hpp"
#include "assert.hpp"
#include "asset_versions.hpp"
#include "assetTypes/gfx_image.hpp"
#include "assetTypes/material.hpp"
#include "assetTypes/model.hpp"
#include "assetTypes/script.hpp"
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
    GetAssetTypeID< Script >::ID();   // AssetType::ASSET_TYPE_SCRIPT
    GetAssetTypeID< Model >::ID();    // AssetType::ASSET_TYPE_MODEL
    PG_ASSERT( GetAssetTypeID< GfxImage >::ID() == 0, "This needs to line up with AssetType ordering" );
    PG_ASSERT( GetAssetTypeID< Material >::ID() == 1, "This needs to line up with AssetType ordering" );
    PG_ASSERT( GetAssetTypeID< Script >::ID() == 2, "This needs to line up with AssetType ordering" );
    PG_ASSERT( GetAssetTypeID< Model >::ID() == 3, "This needs to line up with AssetType ordering" );
    static_assert( NUM_ASSET_TYPES == 4, "Dont forget to add GetAssetTypeID for new assets" );

    Material* defaultMat = new Material;
    defaultMat->name = "default";
    defaultMat->Kd = glm::vec3( 1, .41, .71 ); // hot pink. Material mainly used to bring attention when the intended material is missing
    s_resourceMaps[ASSET_TYPE_MATERIAL]["default"] = defaultMat;
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
        case ASSET_TYPE_GFX_IMAGE:
        {
            GfxImage* asset = new GfxImage;
            if ( !Fastfile_GfxImage_Load( asset, &serializer ) )
            {
                LOG_ERR( "Could not load GfxImage\n" );
                return false;
            }
            auto it = s_resourceMaps[assetType].find( asset->name );
            if ( it == s_resourceMaps[assetType].end() )
            {
                s_resourceMaps[assetType][asset->name] = asset;
            }
            else
            {
                s_resourceMaps[assetType][asset->name]->Move( asset );
            }
            break;
        }
        case ASSET_TYPE_MATERIAL:
        {
            Material* asset = new Material;
            if ( !Fastfile_Material_Load( asset, &serializer ) )
            {
                LOG_ERR( "Could not load Material\n" );
                return false;
            }
            auto it = s_resourceMaps[assetType].find( asset->name );
            if ( it == s_resourceMaps[assetType].end() )
            {
                s_resourceMaps[assetType][asset->name] = asset;
            }
            else
            {
                s_resourceMaps[assetType][asset->name]->Move( asset );
            }
            break;
        }
        case ASSET_TYPE_SCRIPT:
        {
            Script* asset = new Script;
            if ( !Fastfile_Script_Load( asset, &serializer ) )
            {
                LOG_ERR( "Could not load Script\n" );
                return false;
            }
            auto it = s_resourceMaps[assetType].find( asset->name );
            if ( it == s_resourceMaps[assetType].end() )
            {
                s_resourceMaps[assetType][asset->name] = asset;
            }
            else
            {
                s_resourceMaps[assetType][asset->name]->Move( asset );
            }
            break;
        }
        case ASSET_TYPE_MODEL:
        {
            Model* asset = new Model;
            if ( !Fastfile_Model_Load( asset, &serializer ) )
            {
                LOG_ERR( "Could not load Model\n" );
                return false;
            }
            auto it = s_resourceMaps[assetType].find( asset->name );
            if ( it == s_resourceMaps[assetType].end() )
            {
                s_resourceMaps[assetType][asset->name] = asset;
            }
            else
            {
                s_resourceMaps[assetType][asset->name]->Move( asset );
            }
            break;
        }
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