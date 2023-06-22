#include "pt_scene.hpp"
#include "asset/asset_manager.hpp"
#include "asset/pt_model.hpp"
#include "core/lua.hpp"
#include "core/time.hpp"
#include "ecs/component_factory.hpp"
#include "ecs/components/model_renderer.hpp"
#include "intersection_tests.hpp"
#include "shared/assert.hpp"
#include "shared/filesystem.hpp"
#include "shared/json_parsing.hpp"
#include "shared/logger.hpp"

using namespace PG;


namespace PT
{

Scene::~Scene()
{
    for ( auto light : lights )
    {
        delete light;
    }
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


static void ParseBaseLight( const rapidjson::Value& value, Light* light )
{
    struct BaseLightCreateInfo
    {
        glm::vec3 color = glm::vec3( 0 );
        float intensity = 1.0f;
        int nSamples = 1;
    };
    static JSONFunctionMapper< BaseLightCreateInfo& > mapping(
    {
        { "color",     []( const rapidjson::Value& v, BaseLightCreateInfo& l ) { l.color     = ParseVec3( v ); } },
        { "intensity", []( const rapidjson::Value& v, BaseLightCreateInfo& l ) { l.intensity = ParseNumber< float >( v ); } },
        { "nSamples",  []( const rapidjson::Value& v, BaseLightCreateInfo& l ) { l.nSamples  = ParseNumber< int >( v ); } }
    });
    BaseLightCreateInfo info;
    mapping.ForEachMember( value, info );
    light->Lemit = info.intensity * info.color;
    light->nSamples = info.nSamples;
}


static bool ParseDirectionalLight( const rapidjson::Value& value, Scene* scene )
{
    static JSONFunctionMapper< DirectionalLight& > mapping(
    {
        { "direction",  []( const rapidjson::Value& v, DirectionalLight& l ) { l.direction = glm::normalize( ParseVec3( v ) ); } }
    });

    auto light = new DirectionalLight;
    ParseBaseLight( value, light );
    mapping.ForEachMember( value, *light );
    scene->lights.push_back( light );
    return true;
}


static bool ParsePointLight( const rapidjson::Value& value, Scene* scene )
{
    static JSONFunctionMapper< PointLight& > mapping(
    {
        { "position",   []( const rapidjson::Value& v, PointLight& l ) { l.position = ParseVec3( v ); } },
    });

    auto light = new PointLight;
    ParseBaseLight( value, light );
    mapping.ForEachMember( value, *light );
    scene->lights.push_back( light );
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


static bool ParseOfflineRenderSettings( const rapidjson::Value& v, Scene* scene )
{
    static JSONFunctionMapper< RenderSettings& > mapping(
    {
        { "imageResolution", []( const rapidjson::Value& v, RenderSettings& s ) { s.imageResolution = ParseVec2<int>( v ); } },
        { "maxDepth", []( const rapidjson::Value& v, RenderSettings& s ) { s.maxDepth = ParseNumber<int>( v ); } },
        { "outputImageFilename", []( const rapidjson::Value& v, RenderSettings& s ) { s.outputImageFilename = v.GetString(); } },
        { "numSamplesPerPixel", []( const rapidjson::Value& v, RenderSettings& s )
            {
                s.numSamplesPerPixel.clear();
                for ( const rapidjson::Value& item : v.GetArray() )
                {
                    s.numSamplesPerPixel.push_back( item.GetInt() );
                }
            }
        },
        { "antialiasMethod", []( const rapidjson::Value& v, RenderSettings& s ) { s.antialiasMethod = AntiAlias::AlgorithmFromString( v.GetString() ); } },
        { "tonemapMethod", []( const rapidjson::Value& v, RenderSettings& s ) { s.tonemapMethod = TonemapOperatorFromString( v.GetString() ); } }
    });

    mapping.ForEachMember( v, scene->settings );
    return true;
}


static bool ParseSkybox( const rapidjson::Value& v, Scene* scene )
{
    PG_ASSERT( v.IsString() );
    std::string name = v.GetString();
    auto skybox = AssetManager::Get<GfxImage>( name );
    scene->skybox = LoadTextureFromGfxImage( skybox );
    PG_ASSERT( scene->skybox != TEXTURE_HANDLE_INVALID, "Could not find skybox with name '%s'", name.c_str() );
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
    PG_ASSERT( script, "No script found with name '%s'", scrName.c_str() );
    scene->nonEntityScripts.emplace_back( script );
    return true;
}


static void CreateShapesFromSceneGeo( Scene* scene )
{
    scene->registry.view< ModelRenderer, Transform >().each( [&]( ModelRenderer& modelRenderer, Transform& transform )
    {
        AddMeshInstancesForModel( modelRenderer.model, modelRenderer.materials, transform );
    });

    EmitTrianglesForAllMeshes( scene->shapes, scene->lights );
}


bool Scene::Load( const std::string& filename )
{
    rapidjson::Document document;
    if ( !ParseJSONFile( filename, document ) )
    {
        LOG_ERR( "Failed to parse scene" );
        return false;
    }

    Lua::State()["ECS"] = &this->registry;
    Lua::State()["scene"] = this;

    if ( !AssetManager::LoadFastFile( "defaults_required" ) )
    {
        return false;
    }

    if ( !AssetManager::LoadFastFile( GetFilenameStem( filename ) ) )
    {
        return false;
    }

    static JSONFunctionMapperBoolCheck< Scene* > mapping(
    {
        { "Camera",                ParseCamera },
        { "Entity",                ParseEntity },
        { "DirectionalLight",      ParseDirectionalLight },
        { "PointLight",            ParsePointLight },
        { "OfflineRenderSettings", ParseOfflineRenderSettings },
        { "Skybox",                ParseSkybox },
        { "SkyEVAdjust",           ParseSkyEVAdjust },
        { "SkyTint",               ParseSkyTint },
        { "StartupScript",         ParseStartupScript },
        { "Script",                ParseScript },
    });

    Scene* scene = this;
    scene->skybox = TEXTURE_HANDLE_INVALID;
    for ( rapidjson::Value::ConstValueIterator itr = document.Begin(); itr != document.End(); ++itr )
    {
        if ( !mapping.ForEachMember( *itr, scene ) )
        {
            return false;
        }
    }

    Start();
    CreateShapesFromSceneGeo( scene );

    LOG( "Building BVH for %zu shapes...", shapes.size() );
    auto bvhTime = Time::GetTimePoint();
    bvh.Build( shapes );
    float bvhBuildTime = (float)Time::GetDuration( bvhTime ) / 1000.0f;
    LOG( "BVH build time: %.3f seconds", bvhBuildTime );

    return true;
}


void Scene::Start()
{
    Lua::State()["ECS"] = &registry;
    Lua::State()["scene"] = this;

    for ( auto& script : nonEntityScripts )
    {
        sol::function startFn = script.env["Start"];
        if ( startFn.valid() )
        {
            //script.env["scene"] = this;
            //CHECK_SOL_FUNCTION_CALL( startFn() );
        }
    }
}


bool Scene::Intersect( const Ray& ray, IntersectionData& hitData )
{
    hitData.t = FLT_MAX;
    //for ( const auto& shape : bvh.shapes )
    //{
    //    shape->Intersect( ray, &hitData );
    //}
    //return hitData.t != FLT_MAX;
    return bvh.Intersect( ray, &hitData );
}


bool Scene::Occluded( const Ray& ray, float tMax )
{
    //for ( const auto& shape : bvh.shapes )
    //{
    //    if ( shape->TestIfHit( ray, tMax ) )
    //    {
    //        return true;
    //    }
    //}
    //return false;
    return bvh.Occluded( ray, tMax );
}


glm::vec3 Scene::LEnvironment( const Ray& ray )
{
    if ( skybox != TEXTURE_HANDLE_INVALID )
    {
        glm::vec3 radiance = glm::vec3( GetTex( skybox )->SampleDir( ray.direction ) );
        radiance *= skyTint;
        radiance *= std::exp2f( skyEVAdjust );
        return radiance;
    }

    return glm::vec3( 0 );

}

} // namespace PT
