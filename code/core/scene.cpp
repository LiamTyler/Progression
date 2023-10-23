#include "core/scene.hpp"
#include "asset/asset_manager.hpp"
#include "core/lua.hpp"
#include "core/time.hpp"
#include "ecs/component_factory.hpp"
#include "shared/assert.hpp"
#include "shared/filesystem.hpp"
#include "shared/json_parsing.hpp"
#include "shared/logger.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>

using namespace PG;

Scene* s_primaryScene = nullptr;

static bool ParseNull( const rapidjson::Value& v, Scene* scene )
{
    PG_UNUSED( v );
    PG_UNUSED( scene );
    return true;
}


static bool ParseAmbientColor( const rapidjson::Value& v, Scene* scene )
{
    scene->ambientColor = ParseVec3( v );
    return true;
}


static bool ParseCamera( const rapidjson::Value& v, Scene* scene )
{
    Camera& camera = scene->camera;
    static JSONFunctionMapper< Camera& > mapping(
    {
        { "position",    []( const rapidjson::Value& v, Camera& camera ) { camera.position    = ParseVec3( v ); } },
        { "rotation",    []( const rapidjson::Value& v, Camera& camera ) { camera.rotation    = glm::radians( ParseVec3( v ) ); } },
        { "vFov",        []( const rapidjson::Value& v, Camera& camera ) { camera.vFov        = glm::radians( ParseNumber< float >( v ) ); } },
        { "aspectRatio", []( const rapidjson::Value& v, Camera& camera ) { camera.aspectRatio = ParseNumber< float >( v ); } },
        { "nearPlane",   []( const rapidjson::Value& v, Camera& camera ) { camera.nearPlane   = ParseNumber< float >( v ); } },
        { "farPlane",    []( const rapidjson::Value& v, Camera& camera ) { camera.farPlane    = ParseNumber< float >( v ); } },
        { "exposure",    []( const rapidjson::Value& v, Camera& camera ) { camera.exposure    = ParseNumber< float >( v ); } },
    });

    mapping.ForEachMember( v, camera );

    camera.UpdateFrustum();
    camera.UpdateProjectionMatrix();
    return true;
}


static bool ParseDirectionalLight( const rapidjson::Value& value, Scene* scene )
{
    static JSONFunctionMapper< DirectionalLight& > mapping(
    {
        { "color",      [](const rapidjson::Value& v, DirectionalLight& l ) { l.color     = ParseVec3( v ); } },
        { "intensity",  [](const rapidjson::Value& v, DirectionalLight& l ) { l.intensity = ParseNumber< float >( v ); } },
        { "direction",  [](const rapidjson::Value& v, DirectionalLight& l ) { l.direction = glm::normalize( ParseVec3( v ) ); } }
    });

    DirectionalLight& light = scene->directionalLights.emplace_back();
    mapping.ForEachMember( value, light );
    return true;
}


static bool ParseEntity( const rapidjson::Value& v, Scene* scene )
{
    auto e = scene->registry.create();
    for ( auto it = v.MemberBegin(); it != v.MemberEnd(); ++it )
    {
        ParseComponent( it->value, e, scene->registry, it->name.GetString() );
    }
    return true;
}


static bool ParsePointLight( const rapidjson::Value& value, Scene* scene )
{
    static JSONFunctionMapper< PointLight& > mapping(
    {
        { "color",      []( const rapidjson::Value& v, PointLight& l ) { l.color     = ParseVec3( v ); } },
        { "intensity",  []( const rapidjson::Value& v, PointLight& l ) { l.intensity = ParseNumber< float >( v ); } },
        { "position",   []( const rapidjson::Value& v, PointLight& l ) { l.position  = ParseVec3( v ); } },
        { "radius",     []( const rapidjson::Value& v, PointLight& l ) { l.radius    = ParseNumber< float >( v ); } },
    });

    PointLight& light = scene->pointLights.emplace_back();
    mapping.ForEachMember( value, light );
    return true;
}


static bool ParseSpotLight( const rapidjson::Value& value, Scene* scene )
{
    static JSONFunctionMapper< SpotLight& > mapping(
    {
        { "color",      []( const rapidjson::Value& v, SpotLight& l ) { l.color     = ParseVec3( v ); } },
        { "intensity",  []( const rapidjson::Value& v, SpotLight& l ) { l.intensity = ParseNumber< float >( v ); } },
        { "position",   []( const rapidjson::Value& v, SpotLight& l ) { l.position  = ParseVec3( v ); } },
        { "radius",     []( const rapidjson::Value& v, SpotLight& l ) { l.radius    = ParseNumber< float >( v ); } },
        { "direction",  []( const rapidjson::Value& v, SpotLight& l ) { l.direction = glm::normalize( ParseVec3( v ) ); } },
        { "innerAngle", []( const rapidjson::Value& v, SpotLight& l ) { l.innerAngle = glm::radians( ParseNumber< float >( v ) ); } },
        { "outerAngle", []( const rapidjson::Value& v, SpotLight& l ) { l.outerAngle = glm::radians( ParseNumber< float >( v ) ); } },
    });

    SpotLight& light = scene->spotLights.emplace_back();
    mapping.ForEachMember( value, light );
    return true;
}


static bool ParseSkybox( const rapidjson::Value& v, Scene* scene )
{
    PG_ASSERT( v.IsString() );
    std::string name = v.GetString();
    scene->skybox = AssetManager::Get<GfxImage>( name );
    PG_ASSERT( scene->skybox, "Could not find skybox with name '%s'", name.c_str() );
#if USING( GAME )
    scene->skyboxIrradiance = AssetManager::Get<GfxImage>( name + "_irradiance" );
    scene->skyboxReflectionProbe = AssetManager::Get<GfxImage>( name + "_reflectionProbe" );
#endif // #if USING( GAME )

    return true;
}


static bool ParseSkyEVAdjust( const rapidjson::Value& v, Scene* scene )
{
    scene->skyEVAdjust = ParseNumber<float>( v );
    return true;
}


static bool ParseSkyTint( const rapidjson::Value& v, Scene* scene )
{
    scene->skyTint = ParseVec3( v );
    return true;
}


static bool ParseStartupScript( const rapidjson::Value& v, Scene* scene )
{
    PG_ASSERT( v.IsString() );
    std::string s = v.GetString();
    Script* script = AssetManager::Get< Script >( v.GetString() );
    PG_ASSERT( script );
    Lua::RunScriptNow( script->scriptText );
    return true;
}


static bool ParseScript( const rapidjson::Value& v, Scene* scene )
{
    PG_ASSERT( v.IsString() );
    std::string scrName = v.GetString();
    Script* script = AssetManager::Get<Script>( scrName );
#if !USING( CONVERTER )
    PG_ASSERT( script, "No script found with name '%s'", scrName.c_str() );
    if ( scene->numNonEntityScripts == PG_MAX_NON_ENTITY_SCRIPTS )
    {
        LOG_ERR( "Could not add nonEntityScript '%s' because already reached the script limit %d", scrName.c_str(), PG_MAX_NON_ENTITY_SCRIPTS );
        return false;
    }
    scene->nonEntityScripts[scene->numNonEntityScripts++] = Lua::ScriptInstance( script );
#endif // #if !USING( CONVERTER )
    return true;
}


namespace PG
{

Scene::~Scene()
{
#if USING( GPU_DATA )
    //tlas.Free();
#endif // #if USING( GPU_DATA )
}


Scene* Scene::Load( const std::string& filename )
{
    Scene* scene = new Scene;

    rapidjson::Document document;
    if ( !ParseJSONFile( filename, document ) )
    {
        LOG_ERR( "Failed to parse scene" );
        delete scene;
        return nullptr;
    }

    Scene* previousPrimaryScene = GetPrimaryScene();
    SetPrimaryScene( scene );
    Lua::State()["ECS"] = &scene->registry;
    Lua::State()["scene"] = scene;

#if USING( GAME )
    if ( !AssetManager::LoadFastFile( GetFilenameStem( filename ) ) )
    {
        delete scene;
        return nullptr;
    }
#endif // #if USING( GAME )

    static JSONFunctionMapperBoolCheck< Scene* > mapping(
    {
        { "AmbientColor",     ParseAmbientColor },
        { "Camera",           ParseCamera },
        { "Entity",           ParseEntity },
        { "DirectionalLight", ParseDirectionalLight },
        { "OfflineRenderSettings", ParseNull },
        { "PointLight",       ParsePointLight },
        { "SpotLight",        ParseSpotLight },
        { "Skybox",           ParseSkybox },
        { "SkyEVAdjust",      ParseSkyEVAdjust },
        { "SkyTint",          ParseSkyTint },
        { "StartupScript",    ParseStartupScript },
        { "Script",           ParseScript },
    });

    for ( rapidjson::Value::ConstValueIterator itr = document.Begin(); itr != document.End(); ++itr )
    {
        if ( !mapping.ForEachMember( *itr, scene ) )
        {
            delete scene;
            SetPrimaryScene( previousPrimaryScene );
            return nullptr;
        }
    }

    scene->Start();
    return scene;
}


void Scene::Start()
{
    Lua::State()["ECS"] = &registry;
    Lua::State()["scene"] = this;

    for ( int i = 0; i < numNonEntityScripts; ++i )
    {
        sol::function startFn = nonEntityScripts[i].env["Start"];
        if ( startFn.valid() )
        {
            //nonEntityScripts[i].env["scene"] = this;
            CHECK_SOL_FUNCTION_CALL( startFn() );
        }
    }
}


void Scene::Update()
{
    Lua::State()["ECS"] = &registry;
    Lua::State()["scene"] = this;
    auto luaTimeNamespace = Lua::State()["Time"].get< sol::table >();
    luaTimeNamespace["dt"] = Time::DeltaTime();
    for ( int i = 0; i < numNonEntityScripts; ++i )
    {
        if ( nonEntityScripts[i].hasUpdateFunction )
        {
            CHECK_SOL_FUNCTION_CALL( nonEntityScripts[i].updateFunction() );
        }
    }
}


Scene* GetPrimaryScene()
{
    return s_primaryScene;
}


void SetPrimaryScene( Scene* scene )
{
    s_primaryScene = scene;
}


void RegisterLuaFunctions_Scene( lua_State* L )
{
    sol::state_view lua( L );
    sol::usertype< Scene > scene_type = lua.new_usertype< Scene >( "Scene" );
    scene_type["camera"] = &Scene::camera;
}


} // namespace PG
