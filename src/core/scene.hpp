#pragma once

#include "core/camera.hpp"
#include "ecs/ecs.hpp"
#include "core/lights.hpp"
#include <string>
#include <vector>

#define PG_MAX_POINT_LIGHTS 1024
#define PG_MAX_SPOT_LIGHTS 1024

namespace PG
{
    class Image;
    class Scene
    {
    public:
        Scene() = default;
        ~Scene();

        static Scene* Load( const std::string& filename );

        void Start();
        void Update();

        Camera camera;
        glm::vec4 backgroundColor = glm::vec4( 0, 0, 0, 1 );
        glm::vec3 ambientColor    = glm::vec3( .1f );
        DirectionalLight directionalLight;
        PointLight pointLights[PG_MAX_POINT_LIGHTS];
        SpotLight spotLights[PG_MAX_POINT_LIGHTS];
        uint32_t numDirectionalLights = 0;
        uint32_t numPointLights = 0;
        uint32_t numSpotLights  = 0;
        entt::registry registry;
    };

    Scene* GetPrimaryScene();

    void SetPrimaryScene( Scene* scene );

} // namespace PG
