#include "core/lua.hpp"
#include "asset/asset_manager.hpp"
#include "core/camera.hpp"
#include "ecs/ecs.hpp"
#include "core/input.hpp"
#include "core/math.hpp"
#include "core/scene.hpp"
#include "core/time.hpp"
#include "core/window.hpp"

namespace PG
{
namespace Lua
{
    sol::state g_LuaState;

    void Init( lua_State* L )
    {
        sol::state_view state( L );
        state.open_libraries( sol::lib::base, sol::lib::math );

        RegisterLuaFunctions_Math( L );
        RegisterLuaFunctions_Window( L );
        RegisterLuaFunctions_Scene( L );
        RegisterLuaFunctions_Camera( L );
        ECS::RegisterLuaFunctions( L );
        AssetManager::RegisterLuaFunctions( L );
        Time::RegisterLuaFunctions( L );
        Input::RegisterLuaFunctions( L );
    }


    void RunScriptNow( const std::string& script )
    {
        g_LuaState.script( script );
    }


    ScriptInstance::ScriptInstance( Script* inScriptAsset )
    {
        PG_ASSERT( inScriptAsset && !inScriptAsset->scriptText.empty() );
        scriptAsset = inScriptAsset;
        env = sol::environment( g_LuaState, sol::create, g_LuaState.globals() );
        g_LuaState.script( scriptAsset->scriptText, env );
        updateFunction = env["Update"];
        hasUpdateFunction = updateFunction.valid();
    }


    void ScriptInstance::Update()
    {
        if ( hasUpdateFunction )
        {
            updateFunction();
        }
    }

} // namespace Lua
} // namespace PG
