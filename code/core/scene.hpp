#pragma once

#include "core/camera.hpp"
#include "core/lights.hpp"
#include "core/lua.hpp"
#include "ecs/ecs.hpp"
#include <string>
#include <vector>

#define PG_MAX_NON_ENTITY_SCRIPTS 64

namespace PG
{
    struct GfxImage;

    class Scene
    {
    public:
        Scene() = default;
        ~Scene();

        static Scene* Load( const std::string& filename );

        void Start();
        void Update();

        Camera camera;
        glm::vec3 backgroundColor = glm::vec3( 0, 0, 0 );
        glm::vec3 ambientColor    = glm::vec3( .1f );
        DirectionalLight directionalLight;
        std::vector< PointLight > pointLights;
        std::vector< SpotLight > spotLights;
        entt::registry registry;
        GfxImage* skybox = nullptr;

        // scripts that are not a part of the ECS, and not attached to any entity, but can still have per-frame update functions
        Lua::ScriptInstance nonEntityScripts[PG_MAX_NON_ENTITY_SCRIPTS];
        uint16_t numNonEntityScripts = 0;
    };

    Scene* GetPrimaryScene();

    void SetPrimaryScene( Scene* scene );

    void RegisterLuaFunctions_Scene( lua_State* L );

} // namespace PG