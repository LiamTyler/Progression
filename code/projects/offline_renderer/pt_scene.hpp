#pragma once

#include "anti_aliasing.hpp"
#include "bvh.hpp"
#include "core/camera.hpp"
#include "core/lua.hpp"
#include "ecs/ecs.hpp"
#include "pt_lights.hpp"
#include "shapes.hpp"
#include "tonemap.hpp"
#include <vector>

namespace PT
{

struct RenderSettings
{
    std::string outputImageFilename      = "rendered.png";
    ivec2 imageResolution                = ivec2( 400, 400 );
    i32 maxDepth                         = 1;
    std::vector<i32> numSamplesPerPixel  = { 8 };
    AntiAlias::Algorithm antialiasMethod = AntiAlias::Algorithm::NONE;
    TonemapOperator tonemapMethod        = TonemapOperator::ACES;
};

class Scene
{
public:
    Scene() = default;
    ~Scene();

    bool Load( const std::string& filename );
    void Start();
    bool Intersect( const Ray& ray, IntersectionData& hitData );
    bool Occluded( const Ray& ray, f32 tMax = FLT_MAX );
    vec3 LEnvironment( const Ray& ray );

    BVH bvh;
    PG::Camera camera;
    std::vector<Shape*> shapes; // invalid after bvh is built. Use bvh.shapes
    std::vector<Light*> lights;
    vec3 skyTint    = vec3( 1, 1, 1 );
    f32 skyEVAdjust = 0; // scales sky by pow( 2, skyEVAdjust )
    TextureHandle skybox;
    RenderSettings settings = {};

    entt::registry registry;
    std::vector<PG::Lua::ScriptInstance> nonEntityScripts;
};

void RegisterLuaFunctions_PTScene( lua_State* L );

} // namespace PT
