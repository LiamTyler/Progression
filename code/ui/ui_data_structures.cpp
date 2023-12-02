#include "ui/ui_data_structures.hpp"
#include "asset/asset_manager.hpp"

using namespace PG;
using namespace UI;

UIScript::UIScript( sol::state* state, Script* s )
{
    script = s;
    PG_ASSERT( script );
    env = sol::environment( *state, sol::create, state->globals() );
    state->script( script->scriptText, env );
}

UIScript::UIScript( sol::state* state, const std::string& scriptName ) : UIScript( state, AssetManager::Get<Script>( scriptName ) ) {}
