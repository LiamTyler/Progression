#include "offline_scene.hpp"
#include "core/time.hpp"
#include "intersection_tests.hpp"
//#include "resource/resource_manager.hpp"
#include "shared/assert.hpp"
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


bool Scene::Load( const std::string& filename )
{

    //float sceneLoadTime = Time::GetDuration( startTime ) / 1000.0f;
    //LOG( "Building BVH..." );
    //auto bvhTime = Time::GetTimePoint();
    //bvh.Build( shapes );
    //float bvhBuildTime = Time::GetDuration( bvhTime ) / 1000.0f;
    //
    //for ( auto light : lights )
    //{
    //    if ( dynamic_cast< AreaLight* >( light ) )
    //    {
    //        light->nSamples = numSamplesPerAreaLight;
    //    }
    //}

    // compute some scene statistics
    //size_t numSpheres = 0, numTris = 0;
    //size_t numPointLights = 0, numDirectionalLights = 0, numAreaLights = 0;
    //for ( const auto& shape : bvh.shapes )
    //{
    //    if ( std::dynamic_pointer_cast< Sphere >( shape ) ) numSpheres += 1;
    //    else if ( std::dynamic_pointer_cast< Triangle >( shape ) ) numTris += 1;
    //}
    //for ( auto light : lights )
    //{
    //    if ( dynamic_cast< AreaLight* >( light ) ) numAreaLights += 1;
    //    else if ( dynamic_cast< PointLight* >( light ) ) numPointLights += 1;
    //    else if ( dynamic_cast< DirectionalLight* >( light ) ) numDirectionalLights += 1;
    //}
    //LOG( "Scene stats:" );
    //LOG( "------------------------------------------------------" );
    //LOG( "Load time: ", sceneLoadTime, " seconds" );
    //LOG( "BVH build time: ", bvhBuildTime, " seconds" );
    //LOG( "Number of shapes: ", shapes.size() );
    //LOG( "\tSpheres: ", numSpheres );
    //LOG( "\tTriangles: ", numTris );
    //LOG( "Number of lights: ", lights.size() );
    //LOG( "\tAreaLight: ", numAreaLights );
    //LOG( "\tPointLight: ", numPointLights );
    //LOG( "\tDirectionalLights: ", numDirectionalLights );

    return true;
}

bool Scene::Intersect( const Ray& ray, IntersectionData& hitData )
{
    hitData.t = FLT_MAX;
    return bvh.Intersect( ray, &hitData );
    //for ( const auto& shape : bvh.shapes )
    //{
    //    shape->Intersect( ray, &hitData );
    //}
    //return hitData.t != FLT_MAX;
}

bool Scene::Occluded( const Ray& ray, float tMax )
{
    return bvh.Occluded( ray, tMax );
}

glm::vec3 Scene::LEnvironment( const Ray& ray )
{
    if ( skybox )
    {
        return glm::vec3( skybox->GetPixel( ray ) );
    }
    return backgroundRadiance;
}

} // namespace PT
