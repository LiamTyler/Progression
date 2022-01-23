#pragma once

#include "bvh.hpp"
#include "core/camera.hpp"
#include "core/lua.hpp"
#include "ecs/ecs.hpp"
#include "pt_lights.hpp"
#include "shapes.hpp"
#include <vector>

namespace PT
{

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
    
    PG::Camera camera;
    std::vector< Shape* > shapes; // invalid after bvh is built. Use bvh.shapes
    std::vector< Light* > lights;
    glm::vec3 backgroundRadiance    = glm::vec3( 0 );
    //std::shared_ptr< Skybox > skybox;
    std::string outputImageFilename = "rendered.png";
    glm::ivec2 imageResolution      = glm::ivec2( 400, 400 );
    int maxDepth                    = 1;
    std::vector<int> numSamplesPerPixel = { 1 };
    BVH bvh;

    entt::registry registry;
    std::vector<PG::Lua::ScriptInstance> nonEntityScripts;
};

} // namespace PT
