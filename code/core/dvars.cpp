#include "dvars.hpp"
#include "shared/assert.hpp"
#include "shared/math_base.hpp"
#include <cstring>
#include <stdexcept>

#if USING( DEVELOPMENT_BUILD )
#include <fstream>
#endif // #if USING( DEVELOPMENT_BUILD )

namespace PG
{

static std::unordered_map<std::string_view, Dvar*>& DvarMap()
{
    static std::unordered_map<std::string_view, Dvar*> s_dvars;
    return s_dvars;
};

static bool IsNameValid( const char* const inName )
{
    for ( u32 i = 0; i < (u32)strlen( inName ); ++i )
    {
        if ( inName[i] == ' ' || inName[i] == '\t' || inName[i] == '\n' )
            return false;
    }

    return true;
}

static void RegisterDvar( Dvar* dvar )
{
    PG_ASSERT( IsNameValid( dvar->GetName() ), "dvar name '%s' cannot contain any whitespace", dvar->GetName() );
    PG_ASSERT( !DvarMap().contains( dvar->GetName() ), "Trying to register dvar '%s' multiple times! Make sure it's only defined once",
        dvar->GetName() );
    DvarMap().emplace( dvar->GetName(), dvar );
}

const char* Dvar::TypeToString( Type t )
{
    static_assert( Underlying( Type::COUNT ) == 6 );
    switch ( t )
    {
    case Type::BOOL: return "BOOL";
    case Type::INT: return "INT";
    case Type::UINT: return "UINT";
    case Type::FLOAT: return "FLOAT";
    case Type::DOUBLE: return "DOUBLE";
    case Type::VEC4: return "VEC4";
    }

    return "_ERR";
}

Dvar::Dvar( const char* const inName, bool defaultVal, const char* const desc ) : type( Type::BOOL ), name( inName ), description( desc )
{
    value.bVal = defaultVal;
    RegisterDvar( this );
}

Dvar::Dvar( const char* const inName, i32 defaultVal, i32 minVal, i32 maxVal, const char* const desc )
    : type( Type::INT ), minValue( minVal ), maxValue( maxVal ), name( inName ), description( desc )
{
    value.iVal = defaultVal;
    RegisterDvar( this );
}

Dvar::Dvar( const char* const inName, u32 defaultVal, u32 minVal, u32 maxVal, const char* const desc )
    : type( Type::UINT ), minValue( minVal ), maxValue( maxVal ), name( inName ), description( desc )
{
    value.uVal = defaultVal;
    RegisterDvar( this );
}

Dvar::Dvar( const char* const inName, f32 defaultVal, f32 minVal, f32 maxVal, const char* const desc )
    : type( Type::FLOAT ), minValue( minVal ), maxValue( maxVal ), name( inName ), description( desc )
{
    value.fVal = defaultVal;
    RegisterDvar( this );
}

Dvar::Dvar( const char* const inName, f64 defaultVal, f64 minVal, f64 maxVal, const char* const desc )
    : type( Type::DOUBLE ), minValue( minVal ), maxValue( maxVal ), name( inName ), description( desc )
{
    value.dVal = defaultVal;
    RegisterDvar( this );
}

Dvar::Dvar( const char* const inName, vec4 defaultVal, vec4 minVal, vec4 maxVal, const char* const desc )
    : type( Type::VEC4 ), minValue( minVal ), maxValue( maxVal ), name( inName ), description( desc )
{
    value.vVal = defaultVal;
    RegisterDvar( this );
}

const char* const Dvar::GetName() const { return name; }

const char* const Dvar::GetDescription() const { return description; }

bool Dvar::GetBool() const
{
    DVAR_ASSERT( type == Type::BOOL, "Calling GetBool() on a non-bool dvar. Name %s, type %s", name, TypeToString( type ) );
    return value.bVal;
}

i32 Dvar::GetInt() const
{
    DVAR_ASSERT( type == Type::INT, "Calling GetInt() on a non-int dvar. Name %s, type %s", name, TypeToString( type ) );
    return value.iVal;
}

u32 Dvar::GetUint() const
{
    DVAR_ASSERT( type == Type::UINT, "Calling GetUint() on a non-uint dvar. Name %s, type %s", name, TypeToString( type ) );
    return value.uVal;
}

f32 Dvar::GetFloat() const
{
    DVAR_ASSERT( type == Type::FLOAT, "Calling GetFloat() on a non-f32 dvar. Name %s, type %s", name, TypeToString( type ) );
    return value.fVal;
}

f64 Dvar::GetDouble() const
{
    DVAR_ASSERT( type == Type::INT, "Calling GetDouble() on a non-f64 dvar. Name %s, type %s", name, TypeToString( type ) );
    return value.dVal;
}

vec4 Dvar::GetVec4() const
{
    DVAR_ASSERT( type == Type::VEC4, "Calling GetVec4() on a non-vec4 dvar. Name %s, type %s", name, TypeToString( type ) );
    return value.vVal;
}

Dvar::Type Dvar::GetType() const { return type; }

std::string Dvar::GetValueAsString() const
{
    if ( type == Type::BOOL )
        return value.bVal ? "true" : "false";
    else if ( type == Type::INT )
        return std::to_string( value.iVal );
    else if ( type == Type::UINT )
        return std::to_string( value.uVal );
    else if ( type == Type::FLOAT )
        return std::to_string( value.fVal );
    else
        return std::to_string( value.dVal );
}

void Dvar::SetFromString( const std::string& str )
{
    try
    {
        if ( type == Type::BOOL )
        {
            value.bVal = std::stoi( str );
        }
        else if ( type == Type::INT )
        {
            value.iVal = std::stoi( str );
            value.iVal = Min( maxValue.iVal, Max( minValue.iVal, value.iVal ) );
        }
        else if ( type == Type::UINT )
        {
            value.uVal = std::stoul( str );
            value.uVal = Min( maxValue.uVal, Max( minValue.uVal, value.uVal ) );
        }
        else if ( type == Type::FLOAT )
        {
            value.fVal = std::stof( str );
            value.fVal = Min( maxValue.fVal, Max( minValue.fVal, value.fVal ) );
        }
        else if ( type == Type::DOUBLE )
        {
            value.dVal = std::stod( str );
            value.dVal = Min( maxValue.dVal, Max( minValue.dVal, value.dVal ) );
        }
        else if ( type == Type::VEC4 )
        {
            const char* p = str.c_str();
            char* pEnd    = nullptr;
            for ( i32 i = 0; i < 4; ++i )
            {
                f32 x         = strtof( p, &pEnd );
                value.vVal[i] = x;
                if ( p == pEnd )
                    break;
                p = pEnd;
            }
            value.vVal = Min( maxValue.vVal, Max( minValue.vVal, value.vVal ) );
        }
    }
    catch ( const std::invalid_argument& )
    {
        LOG_ERR( "Dvar::SetFromString: could not convert string '%s' into dvar %s of type %s", str.c_str(), name, TypeToString( type ) );
    }
    catch ( const std::out_of_range& )
    {
        LOG_ERR( "Dvar::SetFromString: out of range error when converting string '%s' into dvar %s of type %s", str.c_str(), name,
            TypeToString( type ) );
    }
}

Dvar* GetDvar( std::string_view name ) { return DvarMap().contains( name ) ? DvarMap()[name] : nullptr; }

const std::unordered_map<std::string_view, Dvar*>& GetAllDvars() { return DvarMap(); }

// Make sure this filename lines up with the filename in remote_console_main.cpp
void ExportDvars()
{
#if USING( DEVELOPMENT_BUILD )
    std::ofstream outFile( PG_BIN_DIR "../dvars_exported.bin", std::ios::binary );
    if ( !outFile )
    {
        LOG_ERR( "Failed to open dvar export file '%s'", PG_BIN_DIR "../dvars_exported.bin" );
        return;
    }

    const auto& dvars = DvarMap();
    i32 numDvars      = static_cast<i32>( dvars.size() );
    outFile.write( (char*)&numDvars, sizeof( numDvars ) );
    for ( const auto& [_, dvar] : dvars )
    {
        i32 l = (i32)strlen( dvar->GetName() );
        outFile.write( (char*)&l, sizeof( l ) );
        outFile.write( dvar->GetName(), l );

        l = (i32)strlen( dvar->GetDescription() );
        outFile.write( (char*)&l, sizeof( l ) );
        outFile.write( dvar->GetDescription(), l );
    }
#endif // #if USING( DEVELOPMENT_BUILD )
}

} // namespace PG
