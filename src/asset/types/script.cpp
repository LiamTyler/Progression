#include "asset/types/script.hpp"
#include "core/assert.hpp"
#include "utils/logger.hpp"
#include "utils/serializer.hpp"
#include <fstream>

namespace PG
{


bool Script_Load( Script* script, const ScriptCreateInfo& createInfo )
{
    static_assert( sizeof( Script ) == 2 * sizeof( std::string ) + 8, "Dont forget to update this function when changing Script" );
    PG_ASSERT( script );
    script->name = createInfo.name;
    std::ifstream in( createInfo.filename, std::ios::binary );
    if ( !in )
    {
        LOG_ERR( "Could not load script file '%s'\n", createInfo.filename.c_str() );
        return false;
    }

    in.seekg( 0, std::ios::end );
    size_t size = in.tellg();
    script->scriptText.resize( size );
    in.seekg( 0 );
    in.read( &script->scriptText[0], size ); 

    return true;
}


bool Fastfile_Script_Load( Script* script, Serializer* serializer )
{
    static_assert( sizeof( Script ) == 2 * sizeof( std::string ) + 8, "Dont forget to update this function when changing Script" );
    PG_ASSERT( script && serializer );
    serializer->Read( script->name );
    serializer->Read( script->scriptText );

    return true;
}


bool Fastfile_Script_Save( const Script * const script, Serializer* serializer )
{
    static_assert( sizeof( Script ) == 2 * sizeof( std::string ) + 8, "Dont forget to update this function when changing Script" );
    PG_ASSERT( script && serializer );
    serializer->Write( script->name );
    serializer->Write( script->scriptText );

    return true;
}


} // namespace PG
