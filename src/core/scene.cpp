#include "core/scene.hpp"
#include "asset/asset_manager.hpp"
#include "core/assert.hpp"
//#include "core/lua.hpp"
#include "core/time.hpp"
#include "ecs/component_factory.hpp"
//#include "resource/image.hpp"
#include "utils/json_parsing.hpp"
#include "utils/logger.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>

using namespace PG;

static void ParseFastfile( const rapidjson::Value& v, Scene* scene )
{
    ForEachJSONMember( v, []( const std::string& name, const rapidjson::Value& value )
    {
        if ( name == "filename" )
        {
            PG_ASSERT( value.IsString() );
            std::string fname = value.GetString();
            AssetManager::LoadFastFile( fname );
        }
    });
}


static void ParseCamera( const rapidjson::Value& v, Scene* scene )
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
    });

    mapping.ForEachMember( v, camera );

    camera.UpdateFrustum();
    camera.UpdateProjectionMatrix();
}


static void ParseDirectionalLight( const rapidjson::Value& value, Scene* scene )
{
    static JSONFunctionMapper< DirectionalLight& > mapping(
    {
        { "color",      [](const rapidjson::Value& v, DirectionalLight& l ) { l.color     = ParseVec3( v ); } },
        { "intensity",  [](const rapidjson::Value& v, DirectionalLight& l ) { l.intensity = ParseNumber< float >( v ); } },
        { "direction",  [](const rapidjson::Value& v, DirectionalLight& l ) { l.direction = glm::vec4( glm::normalize( ParseVec3( v ) ), 0 ); } }
    });

    if ( scene->numDirectionalLights > 0 )
    {
        LOG_WARN( "Can't have more than one directional light in a scene, skipping\n" );
        return;
    }

    scene->numDirectionalLights++;
    mapping.ForEachMember( value, scene->directionalLight );
}


static void ParseEntity( const rapidjson::Value& v, Scene* scene )
{
    auto e = scene->registry.create();
    for ( auto it = v.MemberBegin(); it != v.MemberEnd(); ++it )
    {
        ParseComponent( it->value, e, scene->registry, it->name.GetString() );
    }
}


static void ParsePointLight( const rapidjson::Value& value, Scene* scene )
{
    static JSONFunctionMapper< PointLight& > mapping(
    {
        { "color",      []( const rapidjson::Value& v, PointLight& l ) { l.color     = ParseVec3( v ); } },
        { "intensity",  []( const rapidjson::Value& v, PointLight& l ) { l.intensity = ParseNumber< float >( v ); } },
        { "position",   []( const rapidjson::Value& v, PointLight& l ) { l.position = ParseVec3( v ); } },
        { "radius",     []( const rapidjson::Value& v, PointLight& l ) { l.radius = ParseNumber< float >( v ); } },
    });

    if ( scene->numPointLights >= PG_MAX_POINT_LIGHTS )
    {
        LOG_WARN( "Can't have more than %d point lights in a scene, skipping\n", PG_MAX_POINT_LIGHTS );
        return;
    }

    mapping.ForEachMember( value, scene->pointLights[scene->numPointLights] );
    scene->numPointLights++;
}


static void ParseSpotLight( const rapidjson::Value& value, Scene* scene )
{
    static JSONFunctionMapper< SpotLight& > mapping(
    {
        { "color",      []( const rapidjson::Value& v, SpotLight& l ) { l.color     = ParseVec3( v ); } },
        { "intensity",  []( const rapidjson::Value& v, SpotLight& l ) { l.intensity = ParseNumber< float >( v ); } },
        { "position",   []( const rapidjson::Value& v, SpotLight& l ) { l.position  = ParseVec3( v ); } },
        { "radius",     []( const rapidjson::Value& v, SpotLight& l ) { l.radius    = ParseNumber< float >( v ); } },
        { "direction",  []( const rapidjson::Value& v, SpotLight& l ) { l.direction = glm::vec4( glm::normalize( ParseVec3( v ) ), 0 ); } },
        { "cutoff",     []( const rapidjson::Value& v, SpotLight& l ) { l.cutoff    = glm::radians( ParseNumber< float >( v ) ); } }
    });

    if ( scene->numSpotLights >= PG_MAX_SPOT_LIGHTS )
    {
        LOG_WARN( "Can't have more than %d spot lights in a scene, skipping\n", PG_MAX_SPOT_LIGHTS );
        return;
    }

    mapping.ForEachMember( value, scene->spotLights[scene->numSpotLights] );
    scene->numSpotLights++;
}


static void ParseBackgroundColor( const rapidjson::Value& v, Scene* scene )
{
    PG_ASSERT( v.HasMember( "color" ) );
    auto& member           = v["color"];
    scene->backgroundColor = ParseVec4( member );
}


static void ParseAmbientColor( const rapidjson::Value& v, Scene* scene )
{
    PG_ASSERT( v.HasMember( "color" ) );
    auto& member        = v["color"];
    scene->ambientColor = ParseVec3( member );
}


static void ParseSkybox( const rapidjson::Value& v, Scene* scene )
{
    // PG_ASSERT( v.IsString() );
    // scene->skybox = AssetManager::Get< Image >( v.GetString() );
    // PG_ASSERT( scene->skybox, "Could not find skybox with name '" + std::string( v.GetString() ) + "'" );
}


namespace PG
{

Scene::~Scene()
{
}


Scene* Scene::Load( const std::string& filename )
{
    Scene* scene = new Scene;

    rapidjson::Document document;
    if ( !ParseJSONFile( filename, document ) )
    {
        LOG_ERR( "Failed to parse scene\n" );
        delete scene;
        return nullptr;
    }

    static JSONFunctionMapper< Scene* > mapping(
    {
        { "AmbientColor",     ParseAmbientColor },
        { "BackgroundColor",  ParseBackgroundColor },
        { "Camera",           ParseCamera },
        { "Entity",           ParseEntity },
        { "DirectionalLight", ParseDirectionalLight },
        { "PointLight",       ParsePointLight },
        { "SpotLight",        ParseSpotLight },
        { "Fastfile",         ParseFastfile },
        { "Skybox",           ParseSkybox },
    });

    PG_ASSERT( document.HasMember( "Scene" ), "scene file requires a single object 'Scene' that has a list of all scene objects + scene loading commands" );
    const auto& assetList = document["Scene"];
    for ( rapidjson::Value::ConstValueIterator itr = assetList.Begin(); itr != assetList.End(); ++itr )
    {
        mapping.ForEachMember( *itr, scene );
    }

    return scene;
}

void Scene::Start()
{
    /*
    g_LuaState["registry"] = &registry;
    g_LuaState["scene"] = this;
    registry.view< ScriptComponent >().each([]( const entt::entity e, ScriptComponent& comp )
    {
        for ( int i = 0; i < comp.numScripts; ++i )
        {
            auto startFn = comp.scripts[i].env["Start"];
            if ( startFn.valid() )
            {
                comp.scripts[i].env["entity"] = e;
                CHECK_SOL_FUNCTION_CALL( startFn() );
            }
        }
    });
    */
}

void Scene::Update()
{
    /*
    auto luaTimeNamespace = g_LuaState["Time"].get_or_create< sol::table >();
    luaTimeNamespace["dt"] = Time::DeltaTime();
    g_LuaState["registry"] = &registry;
    registry.view< ScriptComponent >().each([]( const entt::entity e, ScriptComponent& comp )
    {
        for ( int i = 0; i < comp.numScriptsWithUpdate; ++i )
        {
            comp.scripts[i].env["entity"] = e;
            CHECK_SOL_FUNCTION_CALL( comp.scripts[i].updateFunc.second() );
        }
    });

    AnimationSystem::Update( this );
    */
}

} // namespace Progression
