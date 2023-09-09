#pragma once

#include "core/camera.hpp"
#include "core/lights.hpp"
#include "core/lua.hpp"
#include "ecs/ecs.hpp"
#if USING( GPU_DATA )
#include "renderer/graphics_api/acceleration_structure.hpp"
#endif // #if USING( GPU_DATA )
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
        glm::vec3 skyTint = glm::vec3( 1, 1, 1 );
        float skyEVAdjust = 0; // scales sky by pow( 2, skyEVAdjust )
        glm::vec3 ambientColor    = glm::vec3( .1f );
        DirectionalLight directionalLight;
        std::vector< PointLight > pointLights;
        std::vector< SpotLight > spotLights;
        entt::registry registry;
        GfxImage* skybox = nullptr;
        GfxImage* skyboxIrradiance = nullptr;

        // scripts that are not a part of the ECS, and not attached to any entity, but can still have per-frame update functions
        Lua::ScriptInstance nonEntityScripts[PG_MAX_NON_ENTITY_SCRIPTS];
        uint16_t numNonEntityScripts = 0;

    #if USING( GPU_DATA )
        Gfx::AccelerationStructure tlas;
    #endif // #if USING( GPU_DATA )
    };

    Scene* GetPrimaryScene();

    void SetPrimaryScene( Scene* scene );

    void RegisterLuaFunctions_Scene( lua_State* L );

} // namespace PG
