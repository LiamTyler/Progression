#include "core/math.hpp"
#include "core/lua.hpp"
#include <iostream>

using namespace glm;

void RegisterLuaFunctions_Math( lua_State* L )
{
    sol::state_view lua( L );

    sol::usertype< vec2 > vec2_type = lua.new_usertype< vec2 >( "vec2", sol::constructors< vec2(), vec2( float ), vec2( const vec2& ), vec2( float, float ) >() );
    vec2_type["x"] = &vec2::x;
	vec2_type["y"] = &vec2::y;
    vec2_type.set_function( sol::meta_function::addition,       []( const vec2& a, const vec2& b ){ return a + b; } );
    vec2_type.set_function( sol::meta_function::subtraction,    []( const vec2& a, const vec2& b ){ return a - b; } );
    vec2_type.set_function( sol::meta_function::multiplication, []( const vec2& a, const vec2& b ){ return a * b; } );
    vec2_type.set_function( sol::meta_function::division,       []( const vec2& a, const vec2& b ){ return a / b; } );
    vec2_type.set_function( sol::meta_function::unary_minus,    []( const vec2& a ){ return -a; } );
    vec2_type.set_function( "scale",        []( float f, const vec2& a ){ return f * a; } );
    vec2_type.set_function( "dot",          []( const vec2& a, const vec2& b ){ return dot( a, b ); } );
    vec2_type.set_function( "length",       []( const vec2& a ){ return length( a ); } );
    vec2_type.set_function( "normalize",    []( const vec2& a ){ return normalize( a ); } );

    sol::usertype< vec3 > vec3_type = lua.new_usertype< vec3 >( "vec3", sol::constructors< vec3(), vec3( float ), vec3( const vec3& ), vec3( float, float, float ) >() );
    vec3_type["x"] = &vec3::x;
	vec3_type["y"] = &vec3::y;
	vec3_type["z"] = &vec3::z;
    vec3_type.set_function( sol::meta_function::addition,       []( const vec3& a, const vec3& b ){ return a + b; } );
    vec3_type.set_function( sol::meta_function::subtraction,    []( const vec3& a, const vec3& b ){ return a - b; } );
    vec3_type.set_function( sol::meta_function::multiplication, []( const vec3& a, const vec3& b ){ return a * b; } );
    vec3_type.set_function( sol::meta_function::division,       []( const vec3& a, const vec3& b ){ return a / b; } );
    vec3_type.set_function( sol::meta_function::unary_minus,    []( const vec3& a ){ return -a; } );
    vec3_type.set_function( "scale",        []( float f, const vec3& a ){ return f * a; } );
    vec3_type.set_function( "dot",          []( const vec3& a, const vec3& b ){ return dot( a, b ); } );
    vec3_type.set_function( "length",       []( const vec3& a ){ return length( a ); } );
    vec3_type.set_function( "normalize",    []( const vec3& a ){ return normalize( a ); } );

    sol::usertype< vec4 > vec4_type = lua.new_usertype< vec4 >( "vec4", sol::constructors< vec4(), vec4( float ), vec4( const vec4& ), vec4( float, float, float, float ) >() );
    vec4_type["x"] = &vec4::x;
	vec4_type["y"] = &vec4::y;
	vec4_type["z"] = &vec4::z;
	vec4_type["w"] = &vec4::w;
    vec4_type.set_function( sol::meta_function::addition,       []( const vec4& a, const vec4& b ){ return a + b; } );
    vec4_type.set_function( sol::meta_function::subtraction,    []( const vec4& a, const vec4& b ){ return a - b; } );
    vec4_type.set_function( sol::meta_function::multiplication, []( const vec4& a, const vec4& b ){ return a * b; } );
    vec4_type.set_function( sol::meta_function::division,       []( const vec4& a, const vec4& b ){ return a / b; } );
    vec4_type.set_function( sol::meta_function::unary_minus,    []( const vec4& a ){ return -a; } );
    vec4_type.set_function( "scale",        []( float f, const vec4& a ){ return f * a; } );
    vec4_type.set_function( "dot",          []( const vec4& a, const vec4& b ){ return dot( a, b ); } );
    vec4_type.set_function( "length",       []( const vec4& a ){ return length( a ); } );
    vec4_type.set_function( "normalize",    []( const vec4& a ){ return normalize( a ); } );

    sol::usertype< mat3 > mat3_type = lua.new_usertype< mat3 >( "mat3", sol::constructors< mat3(), mat3( float ), mat3( const mat3& ) >() );
    mat3_type.set_function( sol::meta_function::index,          []( const mat3& m, const int index ) { return m[index]; } );
    mat3_type.set_function( sol::meta_function::new_index,      []( mat3& m, const int index, const vec3& x ) { m[index] = x; } );
    mat3_type.set_function( sol::meta_function::addition,       []( const mat3& a, const mat3& b ){ return a + b; } );
    mat3_type.set_function( sol::meta_function::subtraction,    []( const mat3& a, const mat3& b ){ return a - b; } );
    mat3_type.set_function( sol::meta_function::multiplication, []( float f, const mat3& a ){ return f * a; } );
    mat3_type.set_function( sol::meta_function::division,       []( const mat3& a, float f ){ return a / f; } );
    mat3_type.set_function( sol::meta_function::unary_minus,    []( const mat3& a ){ return -a; } );

    sol::usertype< mat4 > mat4_type = lua.new_usertype< mat4 >( "mat4", sol::constructors< mat4(), mat4( float ), mat4( const mat4& ) >() );
    mat4_type.set_function( sol::meta_function::index,          []( const mat4& m, const int index ) { return m[index]; } );
    mat4_type.set_function( sol::meta_function::new_index,      []( mat4& m, const int index, const vec4& x ) { m[index] = x; } );
    mat4_type.set_function( sol::meta_function::addition,       []( const mat4& a, const mat4& b ){ return a + b; } );
    mat4_type.set_function( sol::meta_function::subtraction,    []( const mat4& a, const mat4& b ){ return a - b; } );
    mat4_type.set_function( sol::meta_function::multiplication, []( float f, const mat4& a ){ return f * a; } );
    mat4_type.set_function( sol::meta_function::division,       []( const mat4& a, float f ) { return a / f; } );
    mat4_type.set_function( sol::meta_function::unary_minus,    []( const mat4& a ){ return -a; } );
}