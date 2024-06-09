#pragma once

#include "shared/logger.hpp"
#include "shared/math_vec.hpp"
#include "shared/platform_defines.hpp"
#include <unordered_map>

#define DVAR_DEBUGGING USE_IF( USING( DEBUG_BUILD ) )

#if USING( DVAR_DEBUGGING )
#include "shared/assert.hpp"
#include <typeinfo>
#define DVAR_ASSERT( ... ) PG_ASSERT( __VA_ARGS__ )
#else // #if USING( DVAR_DEBUGGING )
#define DVAR_ASSERT( ... ) \
    do                     \
    {                      \
    } while ( 0 )
#endif // #if USING( DVAR_DEBUGGING )

namespace PG
{

class Dvar
{
private:
    union DvarValue
    {
        bool bVal;
        i32 iVal;
        u32 uVal;
        f32 fVal;
        f64 dVal;
        vec4 vVal;

        DvarValue() : dVal( 0 ) {}
        DvarValue( bool v ) : bVal( v ) {}
        DvarValue( i32 v ) : iVal( v ) {}
        DvarValue( u32 v ) : uVal( v ) {}
        DvarValue( f32 v ) : fVal( v ) {}
        DvarValue( f64 v ) : dVal( v ) {}
        DvarValue( vec4 v ) : vVal( v ) {}
    };

public:
    enum class Type : u8
    {
        BOOL,
        INT,
        UINT,
        FLOAT,
        DOUBLE,
        VEC4,

        COUNT
    };

    static const char* TypeToString( Type t );

    explicit Dvar( const char* const inName, bool defaultVal, const char* const desc );
    explicit Dvar( const char* const inName, i32 defaultVal, i32 minVal, i32 maxVal, const char* const desc );
    explicit Dvar( const char* const inName, u32 defaultVal, u32 minVal, u32 maxVal, const char* const desc );
    explicit Dvar( const char* const inName, f32 defaultVal, f32 minVal, f32 maxVal, const char* const desc );
    explicit Dvar( const char* const inName, f64 defaultVal, f64 minVal, f64 maxVal, const char* const desc );
    explicit Dvar( const char* const inName, vec4 defaultVal, vec4 minVal, vec4 maxVal, const char* const desc );

    const char* const GetName() const;
    const char* const GetDescription() const;
    bool GetBool() const;
    i32 GetInt() const;
    u32 GetUint() const;
    f32 GetFloat() const;
    f64 GetDouble() const;
    vec4 GetVec4() const;
    Type GetType() const;
    std::string GetValueAsString() const;

    template <typename T>
    T* GetRawPtr()
    {
        using UT = relaxed_underlying_type<T>::type;
        if constexpr ( std::is_same_v<UT, bool> )
        {
            return &value.bVal;
        }
        else if constexpr ( std::is_same_v<UT, i8> || std::is_same_v<UT, i16> || std::is_same_v<UT, i32> )
        {
            return &value.iVal;
        }
        else if constexpr ( std::is_same_v<UT, u8> || std::is_same_v<UT, u16> || std::is_same_v<UT, u32> )
        {
            return &value.uVal;
        }
        else if constexpr ( std::is_same_v<UT, f32> )
        {
            return &value.fVal;
        }
        else if constexpr ( std::is_same_v<UT, f64> )
        {
            return &value.dVal;
        }
        else if constexpr ( std::is_same_v<UT, vec4> )
        {
            return &value.vVal;
        }
        else
        {
            LOG_ERR( "Unknown type in Dvar::Set: %s vs %s", typeid( UT ).name(), typeid( T ).name() );
        }
    }

    template <typename T>
    void Set( const T& newVal )
    {
        using UT = relaxed_underlying_type<T>::type;
        if constexpr ( std::is_same_v<UT, bool> )
        {
            DVAR_ASSERT( type == Type::BOOL, "Trying to set a BOOL dvar %s with a variable of type %s", name, typeid( UT ).name() );
            value.bVal = newVal;
        }
        else if constexpr ( std::is_same_v<UT, i8> || std::is_same_v<UT, i16> || std::is_same_v<UT, i32> )
        {
            DVAR_ASSERT( type == Type::INT, "Trying to set an INT dvar %s with a variable of type %s", name, typeid( UT ).name() );
            value.iVal = newVal;
        }
        else if constexpr ( std::is_same_v<UT, u8> || std::is_same_v<UT, u16> || std::is_same_v<UT, u32> )
        {
            DVAR_ASSERT( type == Type::UINT, "Trying to set a UINT dvar %s with a variable of type %s", name, typeid( UT ).name() );
            value.uVal = newVal;
        }
        else if constexpr ( std::is_same_v<UT, f32> )
        {
            DVAR_ASSERT( type == Type::FLOAT, "Trying to set a FLOAT dvar %s with a variable of type %s", name, typeid( UT ).name() );
            value.fVal = newVal;
        }
        else if constexpr ( std::is_same_v<UT, f64> )
        {
            DVAR_ASSERT( type == Type::DOUBLE, "Trying to set a DOUBLE dvar %s with a variable of type %s", name, typeid( UT ).name() );
            value.dVal = newVal;
        }
        else if constexpr ( std::is_same_v<UT, vec4> )
        {
            DVAR_ASSERT( type == Type::VEC4, "Trying to set a VEC4 dvar %s with a variable of type %s", name, typeid( UT ).name() );
            value.vVal = newVal;
        }
        else
        {
            LOG_ERR( "Unknown type in Dvar::Set: %s vs %s", typeid( UT ).name(), typeid( T ).name() );
        }
        // TODO: ideally, there would be an else statement here with a static_assert inside of it if none of the IFs above this pass
        //  constexpr if's don't work, but should try again with consteval if from C++23 once compilers support it
    }

    void SetFromString( const std::string& str );

private:
    const Type type;
    DvarValue value;
    const DvarValue minValue = { 0 };
    const DvarValue maxValue = { 0 };
    const char* const name;
    const char* const description;
};

Dvar* GetDvar( std::string_view name );
const std::unordered_map<std::string_view, Dvar*>& GetAllDvars();

#if !USING( SHIP_BUILD )
void ExportDvars();
#endif // !USING( SHIP_BUILD )

} // namespace PG

#if USING( DVAR_DEBUGGING )
#undef DVAR_ASSERT
#endif // #if USING( DVAR_DEBUGGING )
