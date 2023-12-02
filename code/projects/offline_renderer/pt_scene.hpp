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
    glm::ivec2 imageResolution           = glm::ivec2( 400, 400 );
    int maxDepth                         = 1;
    std::vector<int> numSamplesPerPixel  = { 8 };
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
    bool Occluded( const Ray& ray, float tMax = FLT_MAX );
    glm::vec3 LEnvironment( const Ray& ray );

    BVH bvh;
    PG::Camera camera;
    std::vector<Shape*> shapes; // invalid after bvh is built. Use bvh.shapes
    std::vector<Light*> lights;
    glm::vec3 skyTint = glm::vec3( 1, 1, 1 );
    float skyEVAdjust = 0; // scales sky by pow( 2, skyEVAdjust )
    TextureHandle skybox;
    RenderSettings settings = {};

    entt::registry registry;
    std::vector<PG::Lua::ScriptInstance> nonEntityScripts;
};

} // namespace PT
