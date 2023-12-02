#pragma once

#include "asset/types/script.hpp"
#include "shared/logger.hpp"

#if USING( DEBUG_BUILD )
    #define SOL_ALL_SAFETIES_ON 1
#endif // #if USING( DEBUG_BUILD )
#include "sol/sol.hpp"

#if USING( DEBUG_BUILD )
    #define CHECK_SOL_FUNCTION_CALL( statement )                               \
        {                                                                      \
            sol::function_result res = statement;                              \
            if ( !res.valid() )                                                \
            {                                                                  \
                sol::error err = res;                                          \
                LOG_ERR( "Sol function failed with error: '%s'", err.what() ); \
            }                                                                  \
        }
#else // #if USING( DEBUG_BUILD )
    #define CHECK_SOL_FUNCTION_CALL( statement ) statement
#endif // #if USING( DEBUG_BUILD )

namespace PG::Lua
{

void Init();
void SetupStateFunctions( lua_State* state );
void Shutdown();
sol::state& State();

void RunScriptNow( const std::string& script );
void RunFunction( sol::environment& env, std::string_view funcName );
void RunFunctionSafeChecks( sol::environment& env, std::string_view funcName );

struct ScriptInstance
{
    ScriptInstance() = default;
    ScriptInstance( Script* scriptAsset );

    void Update();

    Script* scriptAsset = nullptr;
    sol::environment env;
    sol::function updateFunction;
    bool hasUpdateFunction = false;
};

} // namespace PG::Lua
