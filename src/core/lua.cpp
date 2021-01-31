#include "core/lua.hpp"
#include "asset/asset_manager.hpp"
#include "core/camera.hpp"
#include "ecs/ecs.hpp"
#include "core/input.hpp"
#include "core/math.hpp"
#include "core/scene.hpp"
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

        RegisterLuaFunctions_Math( state );
        RegisterLuaFunctions_Window( state );
        ECS::RegisterLuaFunctions( state );
        AssetManager::RegisterLuaFunctions( state );
    }


    void RunScriptNow( const std::string& script )
    {
        g_LuaState.script( script );
    }

} // namespace Lua
} // namespace PG
