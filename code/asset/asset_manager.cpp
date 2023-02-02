#include "asset/asset_manager.hpp"
#include "asset/asset_versions.hpp"
#include "asset/types/gfx_image.hpp"
#include "asset/types/material.hpp"
#include "asset/types/model.hpp"
#include "asset/types/script.hpp"
#include "asset/types/shader.hpp"
#include "asset/types/ui_layout.hpp"
#include "core/lua.hpp"
#include "shared/assert.hpp"
#include "shared/logger.hpp"
#include "shared/serializer.hpp"
#include <unordered_map>


namespace PG::AssetManager
{

uint32_t GetAssetTypeIDHelper::IDCounter = 0;
std::unordered_map< std::string, BaseAsset* > g_resourceMaps[AssetType::ASSET_TYPE_COUNT];


void Init()
{
    GetAssetTypeID<GfxImage>::ID();     // AssetType::ASSET_TYPE_GFX_IMAGE
    GetAssetTypeID<Material>::ID();     // AssetType::ASSET_TYPE_MATERIAL
    GetAssetTypeID<Script>::ID();       // AssetType::ASSET_TYPE_SCRIPT
    GetAssetTypeID<Model>::ID();        // AssetType::ASSET_TYPE_MODEL
    GetAssetTypeID<Shader>::ID();       // AssetType::ASSET_TYPE_SHADER
    GetAssetTypeID<UILayout>::ID();     // AssetType::ASSET_TYPE_UI_LAYOUT
    PG_ASSERT( GetAssetTypeID<GfxImage>::ID()   == ASSET_TYPE_GFX_IMAGE, "This needs to line up with AssetType ordering" );
    PG_ASSERT( GetAssetTypeID<Material>::ID()   == ASSET_TYPE_MATERIAL, "This needs to line up with AssetType ordering" );
    PG_ASSERT( GetAssetTypeID<Script>::ID()     == ASSET_TYPE_SCRIPT, "This needs to line up with AssetType ordering" );
    PG_ASSERT( GetAssetTypeID<Model>::ID()      == ASSET_TYPE_MODEL, "This needs to line up with AssetType ordering" );
    PG_ASSERT( GetAssetTypeID<Shader>::ID()     == ASSET_TYPE_SHADER, "This needs to line up with AssetType ordering" );
    PG_ASSERT( GetAssetTypeID<UILayout>::ID()   == ASSET_TYPE_UI_LAYOUT, "This needs to line up with AssetType ordering" );
    static_assert( ASSET_TYPE_COUNT == 7, "Dont forget to add GetAssetTypeID for new assets" );
}


template < typename ActualAssetType >
bool LoadAssetFromFastFile( Serializer* serializer, AssetType assetType )
{
    ActualAssetType* asset = new ActualAssetType;
    if ( !asset->FastfileLoad( serializer ) )
    {
        LOG_ERR( "Could not load %s", g_assetNames[assetType] );
        return false;
    }
    auto it = g_resourceMaps[assetType].find( asset->name );
    if ( it == g_resourceMaps[assetType].end() )
    {
        g_resourceMaps[assetType][asset->name] = asset;
    }
    else
    {
        LOG_WARN( "Asset '%s' of type %s has already been loaded. Skipping. (Need to implement asset overwriting/updates still)", asset->name.c_str(), g_assetNames[assetType] );
        asset->Free();
        delete asset;
    }

    return true;
}

#define LOAD_FF_CASE( ASSET_ENUM, actualAssetType ) \
case ASSET_ENUM: \
    if ( !LoadAssetFromFastFile< actualAssetType >( &serializer, assetType ) ) \
    { \
        return false; \
    } \
    break


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
        PG_ASSERT( assetType < AssetType::ASSET_TYPE_COUNT );
        switch ( assetType )
        {
            LOAD_FF_CASE( ASSET_TYPE_GFX_IMAGE, GfxImage );
            LOAD_FF_CASE( ASSET_TYPE_MATERIAL, Material );
            LOAD_FF_CASE( ASSET_TYPE_SCRIPT, Script );
            LOAD_FF_CASE( ASSET_TYPE_MODEL, Model );
            LOAD_FF_CASE( ASSET_TYPE_SHADER, Shader );
        default:
            LOG_ERR( "Unknown asset type '%d'", static_cast< int >( assetType ) );
            return false;
        }
    }   

    return true;
}


void Shutdown()
{
    for ( uint32_t i = 0; i < AssetType::ASSET_TYPE_COUNT; ++i )
    {
        for ( const auto& it : g_resourceMaps[i] )
        {
            it.second->Free();
            delete it.second;
        }
        g_resourceMaps[i].clear();
    }
}


void RegisterLuaFunctions( lua_State* L )
{
    sol::state_view lua( L );
    sol::table assetManagerNamespace = lua.create_named_table( "AssetManager" );
    
    assetManagerNamespace["GetGfxImage"] = []( const std::string& name ) { return AssetManager::Get<GfxImage>( name ); };
    assetManagerNamespace["GetMaterial"] = []( const std::string& name ) { return AssetManager::Get<Material>( name ); };
    assetManagerNamespace["GetScript"]   = []( const std::string& name ) { return AssetManager::Get<Script>( name ); };
    assetManagerNamespace["GetModel"]    = []( const std::string& name ) { return AssetManager::Get<Model>( name ); };

    sol::usertype<Material> mat_type = lua.new_usertype<Material>( "Material" ); //, sol::constructors<Material()>() );
    mat_type["name"]                 = &Material::name;
    mat_type["albedoTint"]           = &Material::albedoTint;
    mat_type["metalnessTint"]        = &Material::metalnessTint;
    //mat_type["roughnessTint"]        = &Material::roughnessTint;
    mat_type["albedoMetalnessImage"] = &Material::albedoMetalnessImage;
    
    sol::usertype<Model> model_type = lua.new_usertype<Model>( "Model" );
    model_type["name"] = &Model::name;
    model_type["meshes"] = &Model::meshes;
    model_type["originalMaterials"] = &Model::originalMaterials;

    sol::usertype<GfxImage> image_type = lua.new_usertype<GfxImage>( "GfxImage" );
    image_type["name"]        = &GfxImage::name;
    image_type["width"]       = &GfxImage::width;
    image_type["height"]      = &GfxImage::height;
    image_type["depth"]       = &GfxImage::depth;
    image_type["mipLevels"]   = &GfxImage::mipLevels;
    image_type["numFaces"]    = &GfxImage::numFaces;
    image_type["pixelFormat"] = &GfxImage::pixelFormat;
    image_type["imageType"]   = &GfxImage::imageType;

    sol::usertype<Script> script_type = lua.new_usertype<Script>( "Script" );
    script_type["name"] = &Script::name;
    script_type["scriptText"] = &Script::scriptText;
}


BaseAsset* Get( uint32_t assetTypeID, const std::string& name )
{
    PG_ASSERT( assetTypeID < AssetType::ASSET_TYPE_COUNT, "Did you forget to update TOTAL_ASSET_TYPES?" );
    auto it = g_resourceMaps[assetTypeID].find( name );
    return it == g_resourceMaps[assetTypeID].end() ? nullptr : it->second;
}

} // namesapce PG::AssetManager
