#include "dvars.hpp"
#include "shared/assert.hpp"
#include <cstring>
#include <stdexcept>
#if !USING( SHIP_BUILD )
#include <fstream>
#endif // #if !USING( SHIP_BUILD )

namespace PG
{

static std::unordered_map<std::string_view, Dvar*> s_dvars;

static bool IsNameValid( const char* const inName )
{
    for ( uint32_t i = 0; i < (uint32_t)strlen( inName ); ++i )
    {
        if ( inName[i] == ' ' || inName[i] == '\t' || inName[i] == '\n' )
            return false;
    }

    return true;
}

static void RegisterDvar( Dvar* dvar )
{
    PG_ASSERT( IsNameValid( dvar->GetName() ), "dvar name '%s' cannot contain any whitespace", dvar->GetName() );
    PG_ASSERT( !s_dvars.contains( dvar->GetName() ), "Trying to register dvar '%s' multiple times! Make sure it's only defined once", dvar->GetName() );
    s_dvars.emplace( dvar->GetName(), dvar );
}

const char* Dvar::TypeToString( DvarType t )
{
    static_assert( Underlying( DvarType::COUNT ) == 5 );
    switch ( t )
    {
    case DvarType::BOOL:
        return "BOOL";
    case DvarType::INT:
        return "INT";
    case DvarType::UINT:
        return "UINT";
    case DvarType::FLOAT:
        return "FLOAT";
    case DvarType::DOUBLE:
        return "DOUBLE";
    }

    return "_ERR";
}

Dvar::Dvar( const char* const inName, bool defaultVal, const char* const desc )
    : type( DvarType::BOOL ), name( inName ), description( desc )
{
    PG_ASSERT( IsNameValid( inName ) );
    value.bVal = defaultVal;
    RegisterDvar( this );
}

Dvar::Dvar( const char* const inName, int defaultVal, int minVal, int maxVal, const char* const desc )
    : type( DvarType::INT ), minValue( minVal ), maxValue( maxVal ), name( inName ), description( desc )
{
    value.iVal = defaultVal;
    RegisterDvar( this );
}

Dvar::Dvar( const char* const inName, uint32_t defaultVal, uint32_t minVal, uint32_t maxVal, const char* const desc )
    : type( DvarType::UINT ), minValue( minVal ), maxValue( maxVal ), name( inName ), description( desc )
{
    value.uVal = defaultVal;
    RegisterDvar( this );
}

Dvar::Dvar( const char* const inName, float defaultVal, float minVal, float maxVal, const char* const desc )
    : type( DvarType::FLOAT ), minValue( minVal ), maxValue( maxVal ), name( inName ), description( desc )
{
    value.fVal = defaultVal;
    RegisterDvar( this );
}

Dvar::Dvar( const char* const inName, double defaultVal, double minVal, double maxVal, const char* const desc )
    : type( DvarType::DOUBLE ), minValue( minVal ), maxValue( maxVal ), name( inName ), description( desc )
{
    value.dVal = defaultVal;
    RegisterDvar( this );
}

const char* const Dvar::GetName() const
{
    return name;
}

const char* const Dvar::GetDescription() const
{
    return description;
}

bool Dvar::GetBool() const
{
#if USING( DVAR_DEBUGGING )
    PG_ASSERT( type == DvarType::BOOL, "Calling GetBool() on a non-bool dvar. Name %s, type %s", name, TypeToString( type ) );
#endif // #if USING( DVAR_DEBUGGING )
    return value.bVal;
}

int Dvar::GetInt() const
{
#if USING( DVAR_DEBUGGING )
    PG_ASSERT( type == DvarType::INT, "Calling GetInt() on a non-int dvar. Name %s, type %s", name, TypeToString( type ) );
#endif // #if USING( DVAR_DEBUGGING )
    return value.iVal;
}

uint32_t Dvar::GetUint() const
{
#if USING( DVAR_DEBUGGING )
    PG_ASSERT( type == DvarType::UINT, "Calling GetUint() on a non-uint dvar. Name %s, type %s", name, TypeToString( type ) );
#endif // #if USING( DVAR_DEBUGGING )
    return value.uVal;
}

float Dvar::GetFloat() const
{
#if USING( DVAR_DEBUGGING )
    PG_ASSERT( type == DvarType::FLOAT, "Calling GetFloat() on a non-float dvar. Name %s, type %s", name, TypeToString( type ) );
#endif // #if USING( DVAR_DEBUGGING )
    return value.fVal;
}

double Dvar::GetDouble() const
{
#if USING( DVAR_DEBUGGING )
    PG_ASSERT( type == DvarType::INT, "Calling GetDouble() on a non-double dvar. Name %s, type %s", name, TypeToString( type ) );
#endif // #if USING( DVAR_DEBUGGING )
    return value.dVal;
}

void Dvar::SetFromString( const std::string& str )
{
    try
    {
        if ( type == DvarType::BOOL )
        {
            value.bVal = std::stoi( str );
        }
        else if ( type == DvarType::INT )
        {
            value.iVal = std::stoi( str );
            value.iVal = std::min( maxValue.iVal, std::max( minValue.iVal, value.iVal ) );
        }
        else if ( type == DvarType::UINT )
        {
            value.uVal = std::stoul( str );
            value.uVal = std::min( maxValue.uVal, std::max( minValue.uVal, value.uVal ) );
        }
        else if ( type == DvarType::FLOAT )
        {
            value.fVal = std::stof( str );
            value.fVal = std::min( maxValue.fVal, std::max( minValue.fVal, value.fVal ) );
        }
        else if ( type == DvarType::DOUBLE )
        {
            value.dVal = std::stod( str );
            value.dVal = std::min( maxValue.dVal, std::max( minValue.dVal, value.dVal ) );
        }
    }
    catch ( const std::invalid_argument& )
    {
        LOG_ERR( "Dvar::SetFromString: could not convert string '%s' into dvar %s of type %s", str.c_str(), name, TypeToString( type ) );
    }
    catch ( const std::out_of_range& )
    {
        LOG_ERR( "Dvar::SetFromString: out of range error when converting string '%s' into dvar %s of type %s", str.c_str(), name, TypeToString( type ) );
    }
}

Dvar* GetDvar( std::string_view name )
{
    return s_dvars.contains( name ) ? s_dvars[name] : nullptr;
}

const std::unordered_map<std::string_view, Dvar*>& GetAllDvars()
{
    return s_dvars;
}

#if !USING( SHIP_BUILD )
// Make sure this filename lines up with the filename in remote_console_main.cpp
void ExportDvars()
{
    std::ofstream outFile( PG_BIN_DIR "../dvars_exported.bin", std::ios::binary );
    if ( !outFile )
    {
        LOG_ERR( "Failed to open dvar export file '%s'", PG_BIN_DIR "../dvars_exported.bin" );
        return;
    }

    int numDvars = static_cast<int>( s_dvars.size() );
    outFile.write( (char*)&numDvars, sizeof( numDvars ) );
    for ( const auto& [_, dvar] : s_dvars )
    {
        int l = (int)strlen( dvar->GetName() );
        outFile.write( (char*)&l, sizeof( l ) );
        outFile.write( dvar->GetName(), l );

        l = (int)strlen( dvar->GetDescription() );
        outFile.write( (char*)&l, sizeof( l ) );
        outFile.write( dvar->GetDescription(), l );
    }
}
#endif // !USING( SHIP_BUILD )

} // namespace PG