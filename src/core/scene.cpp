#include "core/scene.hpp"
#include "asset/asset_manager.hpp"
#include "core/assert.hpp"
#include "core/lua.hpp"
#include "core/time.hpp"
#include "ecs/component_factory.hpp"
//#include "resource/image.hpp"
#include "utils/json_parsing.hpp"
#include "utils/logger.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>

using namespace PG;

Scene* s_primaryScene = nullptr;

static bool ParseFastfile( const rapidjson::Value& v, Scene* scene )
{
    PG_ASSERT( v.IsString() );
    return AssetManager::LoadFastFile( v.GetString() );
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
        { "direction",  [](const rapidjson::Value& v, DirectionalLight& l ) { l.direction = glm::vec4( glm::normalize( ParseVec3( v ) ), 0 ); } }
    });

    if ( scene->numDirectionalLights > 0 )
    {
        LOG_WARN( "Can't have more than one directional light in a scene, skipping" );
        return true;
    }

    scene->numDirectionalLights++;
    mapping.ForEachMember( value, scene->directionalLight );
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
        { "position",   []( const rapidjson::Value& v, PointLight& l ) { l.position = ParseVec3( v ); } },
        { "radius",     []( const rapidjson::Value& v, PointLight& l ) { l.radius = ParseNumber< float >( v ); } },
    });

    if ( scene->numPointLights >= PG_MAX_POINT_LIGHTS )
    {
        LOG_WARN( "Can't have more than %d point lights in a scene, skipping", PG_MAX_POINT_LIGHTS );
        return true;
    }

    mapping.ForEachMember( value, scene->pointLights[scene->numPointLights] );
    scene->numPointLights++;
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
        { "direction",  []( const rapidjson::Value& v, SpotLight& l ) { l.direction = glm::vec4( glm::normalize( ParseVec3( v ) ), 0 ); } },
        { "cutoff",     []( const rapidjson::Value& v, SpotLight& l ) { l.cutoff    = glm::radians( ParseNumber< float >( v ) ); } }
    });

    if ( scene->numSpotLights >= PG_MAX_SPOT_LIGHTS )
    {
        LOG_WARN( "Can't have more than %d spot lights in a scene, skipping", PG_MAX_SPOT_LIGHTS );
        return true;
    }

    mapping.ForEachMember( value, scene->spotLights[scene->numSpotLights] );
    scene->numSpotLights++;
    return true;
}


static bool ParseBackgroundColor( const rapidjson::Value& v, Scene* scene )
{
    PG_ASSERT( v.IsArray() );
    scene->ambientColor = ParseVec4( v );
    return true;
}


static bool ParseAmbientColor( const rapidjson::Value& v, Scene* scene )
{
    PG_ASSERT( v.IsArray() );
    scene->ambientColor = ParseVec3( v );
    return true;
}


static bool ParseSkybox( const rapidjson::Value& v, Scene* scene )
{
    PG_ASSERT( v.IsString() );
    // scene->skybox = AssetManager::Get< Image >( v.GetString() );
    // PG_ASSERT( scene->skybox, "Could not find skybox with name '" + std::string( v.GetString() ) + "'" );
    return false;
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
        LOG_ERR( "Failed to parse scene" );
        delete scene;
        return nullptr;
    }

    Scene* previousPrimaryScene = GetPrimaryScene();
    SetPrimaryScene( scene );
    Lua::g_LuaState["ECS"] = &scene->registry;
    Lua::g_LuaState["scene"] = scene;

    static JSONFunctionMapperBoolCheck< Scene* > mapping(
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
        { "StartupScript",    ParseStartupScript },
    });

    PG_ASSERT( document.HasMember( "Scene" ), "scene file requires a single object 'Scene' that has a list of all scene objects + scene loading commands" );
    const auto& assetList = document["Scene"];
    for ( rapidjson::Value::ConstValueIterator itr = assetList.Begin(); itr != assetList.End(); ++itr )
    {
        if ( !mapping.ForEachMember( *itr, scene ) )
        {
            delete scene;
            SetPrimaryScene( previousPrimaryScene );
            return nullptr;
        }
    }

    return scene;
}


void Scene::Start()
{
}


void Scene::Update()
{
}


Scene* GetPrimaryScene()
{
    return s_primaryScene;
}


void SetPrimaryScene( Scene* scene )
{
    s_primaryScene = scene;
}


} // namespace PG
