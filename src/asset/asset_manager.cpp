#include "asset/asset_manager.hpp"
#include "asset/asset_versions.hpp"
#include "asset/types/gfx_image.hpp"
#include "asset/types/material.hpp"
#include "asset/types/model.hpp"
#include "asset/types/script.hpp"
#include "asset/types/shader.hpp"
#include "core/assert.hpp"
#include "core/lua.hpp"
#include "utils/logger.hpp"
#include "utils/serializer.hpp"
#include <unordered_map>


namespace PG
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
    GetAssetTypeID< Shader >::ID();   // AssetType::ASSET_TYPE_SHADER
    PG_ASSERT( GetAssetTypeID< GfxImage >::ID() == 0, "This needs to line up with AssetType ordering" );
    PG_ASSERT( GetAssetTypeID< Material >::ID() == 1, "This needs to line up with AssetType ordering" );
    PG_ASSERT( GetAssetTypeID< Script >::ID()   == 2, "This needs to line up with AssetType ordering" );
    PG_ASSERT( GetAssetTypeID< Model >::ID()    == 3, "This needs to line up with AssetType ordering" );
    PG_ASSERT( GetAssetTypeID< Shader >::ID()   == 4, "This needs to line up with AssetType ordering" );
    static_assert( NUM_ASSET_TYPES == 5, "Dont forget to add GetAssetTypeID for new assets" );

    Material* defaultMat = new Material;
    defaultMat->name = "default";
    defaultMat->albedo = glm::vec3( 1, .41, .71 ); // hot pink. Material mainly used to bring attention when the intended material is missing
    s_resourceMaps[ASSET_TYPE_MATERIAL]["default"] = defaultMat;
}


bool LoadFastFile( const std::string& fname )
{
    std::string fullName = PG_ASSET_DIR "cache/fastfiles/" + fname + "_v" + std::to_string( PG_FASTFILE_VERSION ) + ".ff";
    Serializer serializer;
    if ( !serializer.OpenForRead( fullName ) )
    {
        LOG_ERR( "Failed to open fastfile '%s'", fullName.c_str() );
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
                LOG_ERR( "Could not load GfxImage" );
                return false;
            }
            auto it = s_resourceMaps[assetType].find( asset->name );
            if ( it == s_resourceMaps[assetType].end() )
            {
                s_resourceMaps[assetType][asset->name] = asset;
            }
            else
            {
                asset->Free();
                delete asset;
            }
            break;
        }
        case ASSET_TYPE_MATERIAL:
        {
            // ASSET_TYPE_MATERIAL in a fastfile actually implies all materials from a pgMat file, which is 1+ materials
            uint16_t numMaterials;
            serializer.Read( numMaterials );
            for ( uint16_t mat = 0; mat < numMaterials; ++mat )
            {
                Material* asset = new Material;
                if ( !Fastfile_Material_Load( asset, &serializer ) )
                {
                    LOG_ERR( "Could not load Material" );
                    return false;
                }
                auto it = s_resourceMaps[assetType].find( asset->name );
                if ( it == s_resourceMaps[assetType].end() )
                {
                    s_resourceMaps[assetType][asset->name] = asset;
                }
                else
                {
                    asset->Free();
                    delete asset;
                }
            }
            break;
        }
        case ASSET_TYPE_SCRIPT:
        {
            Script* asset = new Script;
            if ( !Fastfile_Script_Load( asset, &serializer ) )
            {
                LOG_ERR( "Could not load Script" );
                return false;
            }
            auto it = s_resourceMaps[assetType].find( asset->name );
            if ( it == s_resourceMaps[assetType].end() )
            {
                s_resourceMaps[assetType][asset->name] = asset;
            }
            else
            {
                asset->Free();
                delete asset;
            }
            break;
        }
        case ASSET_TYPE_MODEL:
        {
            Model* asset = new Model;
            if ( !Fastfile_Model_Load( asset, &serializer ) )
            {
                LOG_ERR( "Could not load Model" );
                return false;
            }
            auto it = s_resourceMaps[assetType].find( asset->name );
            if ( it == s_resourceMaps[assetType].end() )
            {
                s_resourceMaps[assetType][asset->name] = asset;
            }
            else
            {
                asset->Free();
                delete asset;
            }
            break;
        }
        case ASSET_TYPE_SHADER:
        {
            Shader* asset = new Shader;
            if ( !Fastfile_Shader_Load( asset, &serializer ) )
            {
                LOG_ERR( "Could not load Shader" );
                return false;
            }
            auto it = s_resourceMaps[assetType].find( asset->name );
            if ( it == s_resourceMaps[assetType].end() )
            {
                s_resourceMaps[assetType][asset->name] = asset;
            }
            else
            {
                asset->Free();
                delete asset;
            }
            break;
        }
        default:
            LOG_ERR( "Unknown asset type '%d'", (int)assetType );
            return false;
        }
    }   

    return true;
}


void Shutdown()
{
    for ( uint32_t i = 0; i < AssetType::NUM_ASSET_TYPES; ++i )
    {
        for ( const auto& it : s_resourceMaps[i] )
        {
            it.second->Free();
            delete it.second;
        }
        s_resourceMaps[i].clear();
    }
}


void RegisterLuaFunctions( lua_State* L )
{
    sol::state_view lua( L );
    sol::table assetManagerNamespace = lua.create_named_table( "AssetManager" );
    
    assetManagerNamespace["GetGfxImage"] = []( const std::string& name ) { return AssetManager::Get< GfxImage >( name ); };
    assetManagerNamespace["GetMaterial"] = []( const std::string& name ) { return AssetManager::Get< Material >( name ); };
    assetManagerNamespace["GetScript"]   = []( const std::string& name ) { return AssetManager::Get< Script >( name ); };
    assetManagerNamespace["GetModel"]    = []( const std::string& name ) { return AssetManager::Get< Model >( name ); };

    sol::usertype< Material > mat_type = lua.new_usertype< Material >( "Material" );
    mat_type["name"]         = &Material::name;
    mat_type["albedo"]       = &Material::albedo;
    mat_type["metalness"]    = &Material::metalness;
    mat_type["roughness"]    = &Material::roughness;
    mat_type["albedoMap"]    = &Material::albedoMap;
    mat_type["metalnessMap"] = &Material::metalnessMap;
    mat_type["roughnessMap"] = &Material::roughnessMap;
    
    sol::usertype< Model > model_type = lua.new_usertype< Model >( "Model" );
    model_type["name"] = &Model::name;
    model_type["meshes"] = &Model::meshes;
    model_type["originalMaterials"] = &Model::originalMaterials;

    sol::usertype< GfxImage > image_type = lua.new_usertype< GfxImage >( "GfxImage" );
    image_type["name"]        = &GfxImage::name;
    image_type["width"]       = &GfxImage::width;
    image_type["height"]      = &GfxImage::height;
    image_type["depth"]       = &GfxImage::depth;
    image_type["mipLevels"]   = &GfxImage::mipLevels;
    image_type["numFaces"]    = &GfxImage::numFaces;
    image_type["pixelFormat"] = &GfxImage::pixelFormat;
    image_type["imageType"]   = &GfxImage::imageType;

    sol::usertype< Script > script_type = lua.new_usertype< Script >( "Script" );
    script_type["name"] = &Script::name;
    script_type["scriptText"] = &Script::scriptText;
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
} // namesapce PG
