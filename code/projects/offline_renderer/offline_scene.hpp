#pragma once

#include "bvh.hpp"
#include "core/camera.hpp"
#include "lights.hpp"
//#include "resource/material.hpp"
#include "shapes.hpp"
//#include "resource/skybox.hpp"
#include <string>
#include <vector>

namespace PT
{

class Scene
{
public:
    Scene() = default;
    ~Scene();

    bool Load( const std::string& filename );

    bool Intersect( const Ray& ray, IntersectionData& hitData );
    bool Occluded( const Ray& ray, float tMax = FLT_MAX );
    glm::vec3 LEnvironment( const Ray& ray );
    
    PG::Camera camera;
    std::vector< std::shared_ptr< Shape > > shapes; // invalid after bvh is built. Use bvh.shapes
    std::vector< Light* > lights;
    glm::vec3 backgroundRadiance    = glm::vec3( 0 );
    //std::shared_ptr< Skybox > skybox;
    std::string outputImageFilename = "rendered.png";
    glm::ivec2 imageResolution      = glm::ivec2( 1280, 720 );
    int maxDepth                    = 5;
    int numSamplesPerAreaLight      = 1;
    std::vector< int > numSamplesPerPixel = { 32 };
    BVH bvh;
};

} // namespace PT
