#pragma once

#include "core/camera.hpp"
#include "core/lights.hpp"
#include "core/lua.hpp"
#include "ecs/ecs.hpp"
#include <string>
#include <vector>

#define PG_MAX_POINT_LIGHTS 1024
#define PG_MAX_SPOT_LIGHTS 1024
#define PG_MAX_NON_ENTITY_SCRIPTS 64

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

        // scripts that are not a part of the ECS, and not attached to any entity, but can still have per-frame update functions
        Lua::ScriptInstance nonEntityScripts[PG_MAX_NON_ENTITY_SCRIPTS];
        uint16_t numNonEntityScripts = 0;
    };

    Scene* GetPrimaryScene();

    void SetPrimaryScene( Scene* scene );

    void RegisterLuaFunctions_Scene( lua_State* L );

} // namespace PG
