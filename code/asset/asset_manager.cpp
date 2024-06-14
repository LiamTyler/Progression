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

namespace PG::AssetManager
{

u32 GetAssetTypeIDHelper::IDCounter = 0;
std::unordered_map<std::string, BaseAsset*> g_resourceMaps[AssetType::ASSET_TYPE_COUNT];

#if USING( ASSET_LIVE_UPDATE )
std::vector<BaseAsset*> s_pendingAssetUpdates[AssetType::ASSET_TYPE_COUNT];
#endif // #if USING( ASSET_LIVE_UPDATE )

void Init()
{
    GetAssetTypeID<GfxImage>::ID(); // AssetType::ASSET_TYPE_GFX_IMAGE
    GetAssetTypeID<Material>::ID(); // AssetType::ASSET_TYPE_MATERIAL
    GetAssetTypeID<Script>::ID();   // AssetType::ASSET_TYPE_SCRIPT
    GetAssetTypeID<Model>::ID();    // AssetType::ASSET_TYPE_MODEL
    GetAssetTypeID<Shader>::ID();   // AssetType::ASSET_TYPE_SHADER
    GetAssetTypeID<UILayout>::ID(); // AssetType::ASSET_TYPE_UI_LAYOUT
    GetAssetTypeID<Pipeline>::ID(); // AssetType::ASSET_TYPE_PIPELINE
    PG_ASSERT( GetAssetTypeID<GfxImage>::ID() == ASSET_TYPE_GFX_IMAGE, "This needs to line up with AssetType ordering" );
    PG_ASSERT( GetAssetTypeID<Material>::ID() == ASSET_TYPE_MATERIAL, "This needs to line up with AssetType ordering" );
    PG_ASSERT( GetAssetTypeID<Script>::ID() == ASSET_TYPE_SCRIPT, "This needs to line up with AssetType ordering" );
    PG_ASSERT( GetAssetTypeID<Model>::ID() == ASSET_TYPE_MODEL, "This needs to line up with AssetType ordering" );
    PG_ASSERT( GetAssetTypeID<Shader>::ID() == ASSET_TYPE_SHADER, "This needs to line up with AssetType ordering" );
    PG_ASSERT( GetAssetTypeID<UILayout>::ID() == ASSET_TYPE_UI_LAYOUT, "This needs to line up with AssetType ordering" );
    PG_ASSERT( GetAssetTypeID<Pipeline>::ID() == ASSET_TYPE_PIPELINE, "This needs to line up with AssetType ordering" );
    PG_ASSERT( ASSET_TYPE_COUNT == 8, "Dont forget to add GetAssetTypeID for new assets" );
}

#if USING( ASSET_LIVE_UPDATE )
bool LiveUpdatesSupported( AssetType type ) { return false; }
#endif // #if USING( ASSET_LIVE_UPDATE )

static void ClearPendingLiveUpdates()
{
#if USING( ASSET_LIVE_UPDATE )
    u32 numIgnoredAssets = 0;
    for ( u32 assetIdx = 0; assetIdx < AssetType::ASSET_TYPE_COUNT; ++assetIdx )
    {
        for ( BaseAsset* newAssetBase : s_pendingAssetUpdates[assetIdx] )
        {

            ++numIgnoredAssets;
            newAssetBase->Free();
            delete newAssetBase;
        }
        s_pendingAssetUpdates[assetIdx].clear();
    }
#endif // #if USING( ASSET_LIVE_UPDATE )
}

void ProcessPendingLiveUpdates()
{
#if USING( ASSET_LIVE_UPDATE )
    u32 numIgnoredAssets = 0;
    for ( u32 assetIdx = 0; assetIdx < AssetType::ASSET_TYPE_COUNT; ++assetIdx )
    {
        for ( BaseAsset* newAssetBase : s_pendingAssetUpdates[assetIdx] )
        {
            if ( !LiveUpdatesSupported( (AssetType)assetIdx ) )
            {
                ++numIgnoredAssets;
                newAssetBase->Free();
                delete newAssetBase;
                continue;
            }

            // LOG( "Performing live update for asset '%s'", newAssetBase->GetName() );
            // if ( assetIdx == AssetType::ASSET_TYPE_SCRIPT )
            //{
            //     Script* oldAsset = Get<Script>( newAssetBase->GetName() );
            //     Script* newAsset = (Script*)newAssetBase;
            //     UI::ReloadScriptIfInUse( oldAsset, newAsset );
            //     oldAsset->Free();
            //     *oldAsset = std::move( *newAsset );
            // }
            // else if ( assetIdx == AssetType::ASSET_TYPE_UI_LAYOUT )
            //{
            //     UILayout* oldAsset = Get<UILayout>( newAssetBase->GetName() );
            //     UILayout* newAsset = (UILayout*)newAssetBase;
            //     UI::ReloadLayoutIfInUse( oldAsset, newAsset );
            //     oldAsset->Free();
            //     *oldAsset = std::move( *newAsset );
            // }
        }
        s_pendingAssetUpdates[assetIdx].clear();
    }

    if ( numIgnoredAssets > 0 )
        LOG_WARN( "Live update failed for %u assets (TODO: implement live update for those types)", numIgnoredAssets );
#endif // #if USING( ASSET_LIVE_UPDATE )
}

std::string DeserializeAssetName( Serializer* serializer, BaseAsset* asset )
{
    std::string assetName;
    u16 assetNameLen;
    serializer->Read( assetNameLen );
    assetName.resize( assetNameLen );
    serializer->Read( &assetName[0], assetNameLen );
    asset->SetName( assetName );

    return assetName;
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
#if USING( ASSET_LIVE_UPDATE )
    else
    {
        s_pendingAssetUpdates[assetType].push_back( asset );
        // LOG_WARN( "Asset '%s' of type %s has already been loaded. Skipping. (Need to implement asset overwriting/updates still)",
        // assetName.c_str(), g_assetNames[assetType] ); asset->Free(); delete asset;
    }
#endif // #if USING( ASSET_LIVE_UPDATE )

    return true;
}

#define LOAD_FF_CASE( ASSET_ENUM, actualAssetType )                              \
    case ASSET_ENUM:                                                             \
        if ( !LoadAssetFromFastFile<actualAssetType>( &serializer, assetType ) ) \
        {                                                                        \
            return false;                                                        \
        }                                                                        \
        break

bool LoadFastFile( const std::string& fname )
{
    PGP_ZONE_SCOPED_FMT( "LoadFastFile %s", fname.c_str() );

    LOG( "Loading fastfile '%s'...", fname.c_str() );
    std::string fullName = PG_ASSET_DIR "cache/fastfiles/" + fname + "_v" + std::to_string( PG_FASTFILE_VERSION ) + ".ff";
    // std::string fullName = "assets/cache/fastfiles/" + fname + "_v" + std::to_string( PG_FASTFILE_VERSION ) + ".ff";
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
            LOAD_FF_CASE( ASSET_TYPE_UI_LAYOUT, UILayout );
            LOAD_FF_CASE( ASSET_TYPE_PIPELINE, Pipeline );
        default: LOG_ERR( "Unknown asset type '%d'", static_cast<i32>( assetType ) ); return false;
        }
    }

    // TODO: Takes a surprisingly long time to close the file (50-100ms for sponza_intel)
    // Edit: looks like ::UnmapViewOfFile is the culprit. Cost for removing pages from the
    // address space just seems to be about ~75us per MB, which matches what some others online say
    // https://randomascii.wordpress.com/2014/12/10/hidden-costs-of-memory-allocation/
    PGP_MANUAL_ZONEN( SerializerClose, "Serializerclose" );
    serializer.Close();
    PGP_MANUAL_ZONE_END( SerializerClose );

#if USING( GAME )
    PG::Gfx::rg.device.FlushUploadRequests();
#endif // #if USING( GAME )

    return true;
}

// Since the render system shutsdown before the asset manager, need to free up any remaining gpu resources
// before/during the render system's shutdown.
void FreeRemainingGpuResources()
{
    ClearPendingLiveUpdates();
    const AssetType typesWithGpuData[] = {
        ASSET_TYPE_GFX_IMAGE, ASSET_TYPE_MATERIAL, ASSET_TYPE_MODEL, ASSET_TYPE_SHADER, ASSET_TYPE_PIPELINE };
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
    for ( u32 i = 0; i < AssetType::ASSET_TYPE_COUNT; ++i )
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
    // mat_type["name"]                 = &Material::name;
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
    PG_ASSERT( assetTypeID < AssetType::ASSET_TYPE_COUNT, "Did you forget to update TOTAL_ASSET_TYPES?" );
    auto it = g_resourceMaps[assetTypeID].find( name );
    return it == g_resourceMaps[assetTypeID].end() ? nullptr : it->second;
}

} // namespace PG::AssetManager
