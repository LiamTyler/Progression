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

namespace PG
{
struct GfxImage;

class Scene
{
public:
    Scene() = default;
    ~Scene();

    void Start();
    void Update();

    std::string name;
    Camera camera;

    vec3 ambientColor                  = vec3( .1f );
    DirectionalLight* directionalLight = nullptr;
    std::vector<Light*> lights;

    vec3 skyTint                    = vec3( 1, 1, 1 );
    f32 skyEVAdjust                 = 0; // scales sky by pow( 2, skyEVAdjust )
    GfxImage* skybox                = nullptr;
    GfxImage* skyboxIrradiance      = nullptr;
    GfxImage* skyboxReflectionProbe = nullptr;

    entt::registry registry;
    // scripts that are not a part of the ECS, and not attached to any entity, but can still have per-frame update functions
    std::vector<Lua::ScriptInstance> nonEntityScripts;

    PointLight* AddPointLight();
    SpotLight* AddSpotLight();
};

void RegisterLuaFunctions_Scene( lua_State* L );

Scene* LoadScene( std::string_view name );
Scene* GetPrimaryScene();
Scene* GetScene( std::string_view name );
void SetPrimaryScene( Scene* scene );
void RemoveScene( Scene* scene );
void RemoveAllScenes();

} // namespace PG
