#include "asset/asset_manager.hpp"
#include "asset/asset_versions.hpp"
#include "core/cpu_profiling.hpp"
#include "core/lua.hpp"
#include "shared/assert.hpp"
#include "shared/logger.hpp"
#include "shared/serializer.hpp"
#include "ui/ui_system.hpp"
#include <unordered_map>

#if USING( GAME )
#include "renderer/r_globals.hpp"
#endif // #if USING( GAME )
#if USING( DEVELOPMENT_BUILD )
#include "shared/filesystem.hpp"
#endif // #if USING( DEVELOPMENT_BUILD )

namespace PG::AssetManager
{

u32 GetAssetTypeIDHelper::IDCounter = 0;
std::unordered_map<std::string, BaseAsset*> g_resourceMaps[ASSET_TYPE_COUNT];

#if USING( ASSET_LIVE_UPDATE )
std::vector<std::pair<BaseAsset*, BaseAsset*>> s_pendingAssetUpdates[ASSET_TYPE_COUNT];
std::vector<AssetLiveUpdateCallbackPtr> s_pendingAssetCallbacks[ASSET_TYPE_COUNT];
#endif // #if USING( ASSET_LIVE_UPDATE )

void Init()
{
    GetAssetTypeID<GfxImage>::ID(); // ASSET_TYPE_GFX_IMAGE
    GetAssetTypeID<Material>::ID(); // ASSET_TYPE_MATERIAL
    GetAssetTypeID<Script>::ID();   // ASSET_TYPE_SCRIPT
    GetAssetTypeID<Model>::ID();    // ASSET_TYPE_MODEL
    GetAssetTypeID<Shader>::ID();   // ASSET_TYPE_SHADER
    GetAssetTypeID<UILayout>::ID(); // ASSET_TYPE_UI_LAYOUT
    GetAssetTypeID<Pipeline>::ID(); // ASSET_TYPE_PIPELINE ASSET_TYPE_FONT
    GetAssetTypeID<Font>::ID();     // ASSET_TYPE_FONT
    PG_ASSERT( GetAssetTypeID<GfxImage>::ID() == ASSET_TYPE_GFX_IMAGE, "This needs to line up with AssetType ordering" );
    PG_ASSERT( GetAssetTypeID<Material>::ID() == ASSET_TYPE_MATERIAL, "This needs to line up with AssetType ordering" );
    PG_ASSERT( GetAssetTypeID<Script>::ID() == ASSET_TYPE_SCRIPT, "This needs to line up with AssetType ordering" );
    PG_ASSERT( GetAssetTypeID<Model>::ID() == ASSET_TYPE_MODEL, "This needs to line up with AssetType ordering" );
    PG_ASSERT( GetAssetTypeID<Shader>::ID() == ASSET_TYPE_SHADER, "This needs to line up with AssetType ordering" );
    PG_ASSERT( GetAssetTypeID<UILayout>::ID() == ASSET_TYPE_UI_LAYOUT, "This needs to line up with AssetType ordering" );
    PG_ASSERT( GetAssetTypeID<Pipeline>::ID() == ASSET_TYPE_PIPELINE, "This needs to line up with AssetType ordering" );
    PG_ASSERT( GetAssetTypeID<Font>::ID() == ASSET_TYPE_FONT, "This needs to line up with AssetType ordering" );
    PG_ASSERT( ASSET_TYPE_COUNT == 9, "Dont forget to add GetAssetTypeID for new assets" );
}

#if USING( ASSET_LIVE_UPDATE )
bool LiveUpdatesSupported( AssetType type ) { return type == ASSET_TYPE_SCRIPT; }
#endif // #if USING( ASSET_LIVE_UPDATE )

static void ClearPendingLiveUpdates()
{
#if USING( ASSET_LIVE_UPDATE )
    for ( u32 assetIdx = 0; assetIdx < ASSET_TYPE_COUNT; ++assetIdx )
    {
        for ( auto [oldAsset, newAsset] : s_pendingAssetUpdates[assetIdx] )
        {
            oldAsset->Free();
            delete oldAsset;
        }
        s_pendingAssetUpdates[assetIdx].clear();
    }
#endif // #if USING( ASSET_LIVE_UPDATE )
}

void ProcessPendingLiveUpdates()
{
#if USING( ASSET_LIVE_UPDATE )
    for ( u32 assetIdx = 0; assetIdx < ASSET_TYPE_COUNT; ++assetIdx )
    {
        if ( s_pendingAssetUpdates[assetIdx].empty() )
            continue;

        for ( const AssetLiveUpdateCallbackPtr& callback : s_pendingAssetCallbacks[assetIdx] )
        {
            callback( s_pendingAssetUpdates[assetIdx] );
        }

        for ( auto [oldAsset, newAsset] : s_pendingAssetUpdates[assetIdx] )
        {
            LOG( "AssetManager: LiveUpdate for '%s' with type %s", newAsset->GetName(), g_assetNames[assetIdx] );
            oldAsset->Free();
            delete oldAsset;
            g_resourceMaps[assetIdx][newAsset->GetName()] = newAsset;
        }
        s_pendingAssetUpdates[assetIdx].clear();
    }
#endif // #if USING( ASSET_LIVE_UPDATE )
}

template <typename ActualAssetType>
bool LoadAssetFromFastFile( Serializer* serializer, AssetType assetType )
{
    ActualAssetType* asset = new ActualAssetType;
    std::string assetName  = DeserializeAssetName( serializer, asset );

    if ( !asset->FastfileLoad( serializer ) )
    {
        LOG_ERR( "Could not load %s", g_assetNames[assetType] );
        return false;
    }
    auto it = g_resourceMaps[assetType].find( assetName );
    if ( it == g_resourceMaps[assetType].end() )
    {
        g_resourceMaps[assetType][assetName] = asset;
    }
    else
    {
#if USING( ASSET_LIVE_UPDATE )
        if ( LiveUpdatesSupported( assetType ) )
        {
            s_pendingAssetUpdates[assetType].emplace_back( it->second, asset );
        }
        else
        {
            LOG_WARN( "AssetManager: cannot do live update for asset '%s' with type %s. Not supported for this type", assetName.c_str(),
                g_assetNames[assetType] );
            asset->Free();
            delete asset;
        }
#else  // #if USING( ASSET_LIVE_UPDATE )
        asset->Free();
        delete asset;
#endif // #else // #if USING( ASSET_LIVE_UPDATE )
    }

    return true;
}

#define LOAD_FF_CASE( ASSET_ENUM, actualAssetType )                              \
    case ASSET_ENUM:                                                             \
        if ( !LoadAssetFromFastFile<actualAssetType>( &serializer, assetType ) ) \
        {                                                                        \
            return false;                                                        \
        }                                                                        \
        break

bool LoadFastFile( const std::string& ffName, bool debugVersion )
{
    std::string absFilename = PG_ASSET_DIR "cache/fastfiles/" + ffName + "_v" + std::to_string( PG_FASTFILE_VERSION ) + ".ff";
#if USING( DEVELOPMENT_BUILD )
    if ( debugVersion )
    {
        if ( !PathExists( absFilename ) )
            return true;
    }
#endif // #if USING( DEVELOPMENT_BUILD )

    PGP_ZONE_SCOPED_FMT( "LoadFastFile %s", ffName.c_str() );

    LOG( "Loading fastfile '%s'...", ffName.c_str() );
    Serializer serializer;
    if ( !serializer.OpenForRead( absFilename ) )
    {
        LOG_ERR( "Failed to open fastfile '%s'", absFilename.c_str() );
        return false;
    }

    while ( serializer.BytesLeft() > 0 )
    {
        AssetType assetType;
        serializer.Read( assetType );
        PG_ASSERT( assetType < ASSET_TYPE_COUNT );
        switch ( assetType )
        {
            LOAD_FF_CASE( ASSET_TYPE_GFX_IMAGE, GfxImage );
            LOAD_FF_CASE( ASSET_TYPE_MATERIAL, Material );
            LOAD_FF_CASE( ASSET_TYPE_SCRIPT, Script );
            LOAD_FF_CASE( ASSET_TYPE_MODEL, Model );
            LOAD_FF_CASE( ASSET_TYPE_SHADER, Shader );
            LOAD_FF_CASE( ASSET_TYPE_UI_LAYOUT, UILayout );
            LOAD_FF_CASE( ASSET_TYPE_PIPELINE, Pipeline );
            LOAD_FF_CASE( ASSET_TYPE_FONT, Font );
        default: LOG_ERR( "Unknown asset type '%d'", static_cast<i32>( assetType ) ); return false;
        }
    }

    // Note: this takes a surprisingly long time to close the file (50-100ms for sponza_intel)
    // Note2: looks like ::UnmapViewOfFile is the culprit. Cost for removing pages from the
    // address space just seems to be about ~75us per MB, which matches what some others online say
    // https://randomascii.wordpress.com/2014/12/10/hidden-costs-of-memory-allocation/
    PGP_MANUAL_ZONEN( SerializerClose, "SerializerClose" );
    serializer.Close();
    PGP_MANUAL_ZONE_END( SerializerClose );

#if USING( GAME )
    PG::Gfx::rg.device.FlushUploadRequests();
#endif // #if USING( GAME )

#if USING( DEVELOPMENT_BUILD )
    if ( !debugVersion )
    {
        return LoadFastFile( ffName + "_debug", true );
    }
#endif // #if USING( DEVELOPMENT_BUILD )

    return true;
}

// Since the render system shutsdown before the asset manager, need to free up any remaining gpu resources
// before/during the render system's shutdown.
void FreeRemainingGpuResources()
{
    ClearPendingLiveUpdates();
    const AssetType typesWithGpuData[] = {
        ASSET_TYPE_GFX_IMAGE, ASSET_TYPE_MATERIAL, ASSET_TYPE_MODEL, ASSET_TYPE_SHADER, ASSET_TYPE_PIPELINE, ASSET_TYPE_FONT };
    for ( u32 i = 0; i < ARRAY_COUNT( typesWithGpuData ); ++i )
    {
        AssetType type = typesWithGpuData[i];
        for ( const auto& it : g_resourceMaps[type] )
        {
            it.second->Free();
            delete it.second;
        }
        g_resourceMaps[type].clear();
    }
}

void Shutdown()
{
    for ( u32 i = 0; i < ASSET_TYPE_COUNT; ++i )
    {
#if USING( ASSET_LIVE_UPDATE )
        s_pendingAssetCallbacks[i].clear();
        s_pendingAssetUpdates[i].clear();
#endif // #if USING( ASSET_LIVE_UPDATE )

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
    mat_type["SetName"]              = &Material::SetName;
    mat_type["albedoTint"]           = &Material::albedoTint;
    mat_type["metalnessTint"]        = &Material::metalnessTint;
    mat_type["roughnessTint"]        = &Material::roughnessTint;
    mat_type["albedoMetalnessImage"] = &Material::albedoMetalnessImage;
    mat_type["normalRoughnessImage"] = &Material::normalRoughnessImage;

    sol::usertype<Model> model_type = lua.new_usertype<Model>( "Model" );
    // model_type["name"]              = &Model::name;
    model_type["meshes"] = &Model::meshes;
    // model_type["originalMaterials"] = &Model::originalMaterials;

    sol::usertype<GfxImage> image_type = lua.new_usertype<GfxImage>( "GfxImage" );
    // image_type["name"]                 = &GfxImage::name;
    image_type["width"]       = &GfxImage::width;
    image_type["height"]      = &GfxImage::height;
    image_type["depth"]       = &GfxImage::depth;
    image_type["mipLevels"]   = &GfxImage::mipLevels;
    image_type["numFaces"]    = &GfxImage::numFaces;
    image_type["pixelFormat"] = &GfxImage::pixelFormat;
    image_type["imageType"]   = &GfxImage::imageType;

    sol::usertype<Script> script_type = lua.new_usertype<Script>( "Script" );
    // script_type["name"]               = &Script::name;
    script_type["scriptText"] = &Script::scriptText;
}

BaseAsset* Get( u32 assetTypeID, const std::string& name )
{
    PG_ASSERT( assetTypeID < ASSET_TYPE_COUNT, "Did you forget to update TOTAL_ASSET_TYPES?" );
    auto it = g_resourceMaps[assetTypeID].find( name );
    return it == g_resourceMaps[assetTypeID].end() ? nullptr : it->second;
}

} // namespace PG::AssetManager
