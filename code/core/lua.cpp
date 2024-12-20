#include "asset/asset_manager.hpp"
#include "core/camera.hpp"
#include "core/lights.hpp"
#include "core/scene.hpp"
#include "core/time.hpp"
#include "ecs/ecs.hpp"
#include "shared/filesystem.hpp"
#include "shared/math.hpp"
#if USING( GAME )
#include "core/input.hpp"
#include "core/window.hpp"
#endif // #if USING( GAME )
#if USING( OFFLINE_RENDERER )
#include "pt_scene.hpp"
#endif // USING( OFFLINE_RENDERER )

static sol::state* g_LuaState = nullptr;

namespace PG::Lua
{

void RegisterLuaFunctions_Time( lua_State* L )
{
    sol::state_view state( L );
    auto luaTimeNamespace              = state["Time"].get_or_create<sol::table>();
    luaTimeNamespace["dt"]             = 0;
    luaTimeNamespace["Time"]           = Time::Time;
    luaTimeNamespace["GetTimePoint"]   = Time::GetTimePoint;
    luaTimeNamespace["GetTimeSince"]   = Time::GetTimeSince;
    luaTimeNamespace["GetElapsedTime"] = Time::GetElapsedTime;
}

void RegisterLuaFunctions_Math( lua_State* L )
{
    sol::state_view lua( L );

    sol::usertype<vec2> vec2_type =
        lua.new_usertype<vec2>( "vec2", sol::constructors<vec2(), vec2( f32 ), vec2( const vec2& ), vec2( f32, f32 )>() );
    vec2_type["x"] = &vec2::x;
    vec2_type["y"] = &vec2::y;
    vec2_type.set_function( sol::meta_function::addition, []( const vec2& a, const vec2& b ) { return a + b; } );
    vec2_type.set_function( sol::meta_function::subtraction, []( const vec2& a, const vec2& b ) { return a - b; } );
    vec2_type.set_function( sol::meta_function::multiplication, []( const vec2& a, const vec2& b ) { return a * b; } );
    vec2_type.set_function( sol::meta_function::division, []( const vec2& a, const vec2& b ) { return a / b; } );
    vec2_type.set_function( sol::meta_function::unary_minus, []( const vec2& a ) { return -a; } );
    vec2_type.set_function( "scale", []( f32 f, const vec2& a ) { return f * a; } );
    vec2_type.set_function( "dot", []( const vec2& a, const vec2& b ) { return Dot( a, b ); } );
    vec2_type.set_function( "length", []( const vec2& a ) { return Length( a ); } );
    vec2_type.set_function( "normalize", []( const vec2& a ) { return Normalize( a ); } );
    lua["Vec2Lerp"] = []( const vec2& a, const vec2& b, f32 t ) { return ( 1 - t ) * a + t * b; };

    sol::usertype<vec3> vec3_type =
        lua.new_usertype<vec3>( "vec3", sol::constructors<vec3(), vec3( f32 ), vec3( const vec3& ), vec3( f32, f32, f32 )>() );
    vec3_type["x"] = &vec3::x;
    vec3_type["y"] = &vec3::y;
    vec3_type["z"] = &vec3::z;
    vec3_type.set_function( sol::meta_function::addition, []( const vec3& a, const vec3& b ) { return a + b; } );
    vec3_type.set_function( sol::meta_function::subtraction, []( const vec3& a, const vec3& b ) { return a - b; } );
    vec3_type.set_function( sol::meta_function::multiplication, []( const vec3& a, const vec3& b ) { return a * b; } );
    vec3_type.set_function( sol::meta_function::division, []( const vec3& a, const vec3& b ) { return a / b; } );
    vec3_type.set_function( sol::meta_function::unary_minus, []( const vec3& a ) { return -a; } );
    vec3_type.set_function( "scale", []( f32 f, const vec3& a ) { return f * a; } );
    vec3_type.set_function( "dot", []( const vec3& a, const vec3& b ) { return Dot( a, b ); } );
    vec3_type.set_function( "length", []( const vec3& a ) { return Length( a ); } );
    vec3_type.set_function( "normalize", []( const vec3& a ) { return Normalize( a ); } );
    lua["Vec3Lerp"] = []( const vec3& a, const vec3& b, f32 t ) { return ( 1 - t ) * a + t * b; };

    sol::usertype<vec4> vec4_type =
        lua.new_usertype<vec4>( "vec4", sol::constructors<vec4(), vec4( f32 ), vec4( const vec4& ), vec4( f32, f32, f32, f32 )>() );
    vec4_type["x"] = &vec4::x;
    vec4_type["y"] = &vec4::y;
    vec4_type["z"] = &vec4::z;
    vec4_type["w"] = &vec4::w;
    vec4_type.set_function( sol::meta_function::addition, []( const vec4& a, const vec4& b ) { return a + b; } );
    vec4_type.set_function( sol::meta_function::subtraction, []( const vec4& a, const vec4& b ) { return a - b; } );
    vec4_type.set_function( sol::meta_function::multiplication, []( const vec4& a, const vec4& b ) { return a * b; } );
    vec4_type.set_function( sol::meta_function::division, []( const vec4& a, const vec4& b ) { return a / b; } );
    vec4_type.set_function( sol::meta_function::unary_minus, []( const vec4& a ) { return -a; } );
    vec4_type.set_function( "scale", []( f32 f, const vec4& a ) { return f * a; } );
    vec4_type.set_function( "dot", []( const vec4& a, const vec4& b ) { return Dot( a, b ); } );
    vec4_type.set_function( "length", []( const vec4& a ) { return Length( a ); } );
    vec4_type.set_function( "normalize", []( const vec4& a ) { return Normalize( a ); } );
    lua["Vec4Lerp"] = []( const vec4& a, const vec4& b, f32 t ) { return ( 1 - t ) * a + t * b; };

    sol::usertype<mat3> mat3_type = lua.new_usertype<mat3>( "mat3", sol::constructors<mat3(), mat3( f32 ), mat3( const mat3& )>() );
    mat3_type.set_function( sol::meta_function::index, []( const mat3& m, const i32 index ) { return m[index]; } );
    mat3_type.set_function( sol::meta_function::new_index, []( mat3& m, const i32 index, const vec3& x ) { m[index] = x; } );
    mat3_type.set_function( sol::meta_function::addition, []( const mat3& a, const mat3& b ) { return a + b; } );
    mat3_type.set_function( sol::meta_function::subtraction, []( const mat3& a, const mat3& b ) { return a - b; } );
    mat3_type.set_function( sol::meta_function::multiplication, []( f32 f, const mat3& a ) { return f * a; } );
    mat3_type.set_function( sol::meta_function::division, []( const mat3& a, f32 f ) { return a / f; } );
    mat3_type.set_function( sol::meta_function::unary_minus, []( const mat3& a ) { return -a; } );

    sol::usertype<mat4> mat4_type = lua.new_usertype<mat4>( "mat4", sol::constructors<mat4(), mat4( f32 ), mat4( const mat4& )>() );
    mat4_type.set_function( sol::meta_function::index, []( const mat4& m, const i32 index ) { return m[index]; } );
    mat4_type.set_function( sol::meta_function::new_index, []( mat4& m, const i32 index, const vec4& x ) { m[index] = x; } );
    mat4_type.set_function( sol::meta_function::addition, []( const mat4& a, const mat4& b ) { return a + b; } );
    mat4_type.set_function( sol::meta_function::subtraction, []( const mat4& a, const mat4& b ) { return a - b; } );
    mat4_type.set_function( sol::meta_function::multiplication, []( f32 f, const mat4& a ) { return f * a; } );
    mat4_type.set_function( sol::meta_function::division, []( const mat4& a, f32 f ) { return a / f; } );
    mat4_type.set_function( sol::meta_function::unary_minus, []( const mat4& a ) { return -a; } );
}

void RegisterLuaFunctions_Filesystem( lua_State* L )
{
    sol::state_view lua( L );
    lua["GetFilesProjectSubdir"] = []( const std::string& relPath, bool recursive )
    { return GetFilesInDir( PG_ROOT_DIR + relPath, recursive ); };
    lua["GetRelativeFilename"] = GetRelativeFilename;
}

void Init()
{
    PG_ASSERT( !g_LuaState );
    g_LuaState = new sol::state;
    SetupStateFunctions( *g_LuaState );
}

void SetupStateFunctions( lua_State* state )
{
    sol::state_view stateView( state );
    stateView.open_libraries( sol::lib::base, sol::lib::math, sol::lib::string );

    RegisterLuaFunctions_Math( state );
    RegisterLuaFunctions_Scene( state );
#if USING( OFFLINE_RENDERER )
    PT::RegisterLuaFunctions_PTScene( state );
#endif // USING( OFFLINE_RENDERER )
    RegisterLuaFunctions_Camera( state );
    RegisterLuaFunctions_Lights( state );
    ECS::RegisterLuaFunctions( state );
    AssetManager::RegisterLuaFunctions( state );
    RegisterLuaFunctions_Time( state );
    RegisterLuaFunctions_Filesystem( state );
#if USING( GAME )
    RegisterLuaFunctions_Window( state );
    Input::RegisterLuaFunctions( state );
#endif // #if USING( GAME )
}

void Shutdown()
{
    PG_ASSERT( g_LuaState );
    delete g_LuaState;
    g_LuaState = nullptr;
}

sol::state& State() { return *g_LuaState; }

void RunScriptNow( const std::string& script ) { g_LuaState->script( script ); }

void RunFunction( sol::environment& env, std::string_view funcName ) { env[funcName](); }

void RunFunctionSafeChecks( sol::environment& env, std::string_view funcName )
{
    sol::function func = env[funcName];
    if ( func.valid() )
    {
        CHECK_SOL_FUNCTION_CALL( func() );
    }
}

ScriptInstance::ScriptInstance( Script* inScriptAsset )
{
    PG_ASSERT( inScriptAsset );
    scriptAsset = inScriptAsset;
    env         = sol::environment( *g_LuaState, sol::create, g_LuaState->globals() );
    if ( !scriptAsset->scriptText.empty() )
    {
        g_LuaState->script( scriptAsset->scriptText, env );
        updateFunction = env["Update"];
    }
    hasUpdateFunction = updateFunction.valid();
}

void ScriptInstance::Update()
{
    if ( hasUpdateFunction )
    {
        updateFunction();
    }
}

} // namespace PG::Lua
