#include "core/scene.hpp"
#include "asset/asset_manager.hpp"
#include "core/cpu_profiling.hpp"
#include "core/lua.hpp"
#include "core/time.hpp"
#include "ecs/component_factory.hpp"
#include "shared/assert.hpp"
#include "shared/filesystem.hpp"
#include "shared/json_parsing.hpp"
#include "shared/logger.hpp"
#include <algorithm>
#include <fstream>
#include <mutex>
#include <sstream>

using namespace PG;

static Scene* s_primaryScene = nullptr;
static std::vector<Scene*> s_scenes;
static std::mutex s_scenesLock;

// clang-format off

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
        { "rotation",    []( const rapidjson::Value& v, Camera& camera ) { camera.rotation    = DegToRad( ParseVec3( v ) ); } },
        { "vFov",        []( const rapidjson::Value& v, Camera& camera ) { camera.vFov        = DegToRad( ParseNumber< f32 >( v ) ); } },
        { "aspectRatio", []( const rapidjson::Value& v, Camera& camera ) { camera.aspectRatio = ParseNumber< f32 >( v ); } },
        { "nearPlane",   []( const rapidjson::Value& v, Camera& camera ) { camera.nearPlane   = ParseNumber< f32 >( v ); } },
        { "farPlane",    []( const rapidjson::Value& v, Camera& camera ) { camera.farPlane    = ParseNumber< f32 >( v ); } },
        { "exposure",    []( const rapidjson::Value& v, Camera& camera ) { camera.exposure    = ParseNumber< f32 >( v ); } },
    });

    mapping.ForEachMember( v, camera );

    camera.Update();
    return true;
}


static bool ParseDirectionalLight( const rapidjson::Value& value, Scene* scene )
{
    static JSONFunctionMapper< DirectionalLight& > mapping(
    {
        { "color",      [](const rapidjson::Value& v, DirectionalLight& l ) { l.color     = ParseVec3( v ); } },
        { "intensity",  [](const rapidjson::Value& v, DirectionalLight& l ) { l.intensity = ParseNumber< f32 >( v ); } },
        { "direction",  [](const rapidjson::Value& v, DirectionalLight& l ) { l.direction = Normalize( ParseVec3( v ) ); } }
    });

    if ( scene->directionalLight )
    {
        LOG_WARN( "Scene already has a directional light! Only 1 allowed. Skipping this one" );
        return true;
    }
    scene->directionalLight = new DirectionalLight;

    mapping.ForEachMember( value, *scene->directionalLight );
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
        { "intensity",  []( const rapidjson::Value& v, PointLight& l ) { l.intensity = ParseNumber< f32 >( v ); } },
        { "position",   []( const rapidjson::Value& v, PointLight& l ) { l.position  = ParseVec3( v ); } },
        { "radius",     []( const rapidjson::Value& v, PointLight& l ) { l.radius    = ParseNumber< f32 >( v ); } },
    });

    PointLight* light = new PointLight;
    scene->lights.emplace_back( light );
    mapping.ForEachMember( value, *light );
    return true;
}


static bool ParseSpotLight( const rapidjson::Value& value, Scene* scene )
{
    static JSONFunctionMapper< SpotLight& > mapping(
    {
        { "color",      []( const rapidjson::Value& v, SpotLight& l ) { l.color     = ParseVec3( v ); } },
        { "intensity",  []( const rapidjson::Value& v, SpotLight& l ) { l.intensity = ParseNumber< f32 >( v ); } },
        { "position",   []( const rapidjson::Value& v, SpotLight& l ) { l.position  = ParseVec3( v ); } },
        { "radius",     []( const rapidjson::Value& v, SpotLight& l ) { l.radius    = ParseNumber< f32 >( v ); } },
        { "direction",  []( const rapidjson::Value& v, SpotLight& l ) { l.direction = Normalize( ParseVec3( v ) ); } },
        { "innerAngle", []( const rapidjson::Value& v, SpotLight& l ) { l.innerAngle = DegToRad( ParseNumber< f32 >( v ) ); } },
        { "outerAngle", []( const rapidjson::Value& v, SpotLight& l ) { l.outerAngle = DegToRad( ParseNumber< f32 >( v ) ); } },
    });

    SpotLight* light = new SpotLight;
    scene->lights.emplace_back( light );
    mapping.ForEachMember( value, *light );
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
    scene->skyEVAdjust = ParseNumber<f32>( v );
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
    scene->nonEntityScripts.emplace_back( script );
#endif // #if !USING( CONVERTER )
    return true;
}
// clang-format on

namespace PG
{

Scene::~Scene()
{
    for ( size_t i = 0; i < nonEntityScripts.size(); ++i )
    {
        sol::function endFn = nonEntityScripts[i].env["End"];
        if ( endFn.valid() )
        {
            CHECK_SOL_FUNCTION_CALL( endFn() );
        }
    }

    for ( Light* light : lights )
        delete light;

#if USING( GPU_DATA )
        // tlas.Free();
#endif // #if USING( GPU_DATA )
}

Scene* Scene::Load( const std::string& filename )
{
    PGP_ZONE_SCOPEDN( "Scene::Load" );
    Scene* scene = new Scene;
    scene->name  = GetFilenameStem( filename );
    rapidjson::Document document;
    if ( !ParseJSONFile( filename, document ) )
    {
        LOG_ERR( "Failed to parse scene" );
        delete scene;
        return nullptr;
    }

    Scene* previousPrimaryScene = GetPrimaryScene();
    SetPrimaryScene( scene );
    Lua::State()["ECS"]   = &scene->registry;
    Lua::State()["scene"] = scene;

#if USING( GAME )
    auto startTime = Time::GetTimePoint();
    if ( !AssetManager::LoadFastFile( GetFilenameStem( filename ) ) )
    {
        delete scene;
        return nullptr;
    }
    LOG( "Scene %s loaded in %.3f sec", scene->name.c_str(), Time::GetTimeSince( startTime ) / 1000.0f );
#endif // #if USING( GAME )

    static JSONFunctionMapperBoolCheck<Scene*> mapping( {
        {"AmbientColor",          ParseAmbientColor    },
        {"Camera",                ParseCamera          },
        {"Entity",                ParseEntity          },
        {"DirectionalLight",      ParseDirectionalLight},
        {"OfflineRenderSettings", ParseNull            },
        {"PointLight",            ParsePointLight      },
        {"SpotLight",             ParseSpotLight       },
        {"Skybox",                ParseSkybox          },
        {"SkyEVAdjust",           ParseSkyEVAdjust     },
        {"SkyTint",               ParseSkyTint         },
        {"StartupScript",         ParseStartupScript   },
        {"Script",                ParseScript          },
    } );

    for ( rapidjson::Value::ConstValueIterator itr = document.Begin(); itr != document.End(); ++itr )
    {
        if ( !mapping.ForEachMember( *itr, scene ) )
        {
            delete scene;
            SetPrimaryScene( previousPrimaryScene );
            return nullptr;
        }
    }
    s_scenesLock.lock();
    s_scenes.push_back( scene );
    s_scenesLock.unlock();

    scene->Start();
    return scene;
}

void Scene::Start()
{
    Lua::State()["ECS"]   = &registry;
    Lua::State()["scene"] = this;

    for ( size_t i = 0; i < nonEntityScripts.size(); ++i )
    {
        sol::function startFn = nonEntityScripts[i].env["Start"];
        if ( startFn.valid() )
        {
            CHECK_SOL_FUNCTION_CALL( startFn() );
        }
    }
}

void Scene::Update()
{
    PGP_ZONE_SCOPEDN( "Scene::Update" );
    Lua::State()["ECS"]    = &registry;
    Lua::State()["scene"]  = this;
    auto luaTimeNamespace  = Lua::State()["Time"].get<sol::table>();
    luaTimeNamespace["dt"] = Time::DeltaTime();
    for ( size_t i = 0; i < nonEntityScripts.size(); ++i )
    {
        if ( nonEntityScripts[i].hasUpdateFunction )
        {
            CHECK_SOL_FUNCTION_CALL( nonEntityScripts[i].updateFunction() );
        }
    }
}

void RegisterLuaFunctions_Scene( lua_State* L )
{
    sol::state_view lua( L );
    sol::usertype<Scene> scene_type = lua.new_usertype<Scene>( "Scene" );
    scene_type["camera"]            = &Scene::camera;
}

Scene* GetPrimaryScene() { return s_primaryScene; }

Scene* GetScene( std::string_view name )
{
    for ( Scene* scene : s_scenes )
    {
        if ( scene->name == name )
            return scene;
    }

    return nullptr;
}

void SetPrimaryScene( Scene* scene ) { s_primaryScene = scene; }

void FreeScene( Scene* toDeleteScene )
{
    s_scenesLock.lock();
    if ( toDeleteScene == s_primaryScene )
        s_primaryScene = nullptr;
    size_t numElementsErased = std::erase( s_scenes, toDeleteScene );
    delete toDeleteScene;
    s_scenesLock.unlock();
    PG_ASSERT( numElementsErased == 1, "Trying to free a scene that doesn't exist in the managed list" );
}

void FreeAllScenes()
{
    s_scenesLock.lock();
    for ( Scene* scene : s_scenes )
        delete scene;
    s_scenes.clear();
    s_primaryScene = nullptr;
    s_scenesLock.unlock();
}

} // namespace PG
