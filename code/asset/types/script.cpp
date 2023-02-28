#include "asset/types/script.hpp"
#include "shared/assert.hpp"
#include "shared/logger.hpp"
#include "shared/serializer.hpp"
#include <fstream>

namespace PG
{

std::string GetAbsPath_ScriptFilename( const std::string& filename )
{
    return PG_ASSET_DIR + filename;
}


bool Script::Load( const BaseAssetCreateInfo* baseInfo )
{
    static_assert( sizeof( Script ) == 2 * sizeof( std::string ) + 8, "Dont forget to update this function when changing Script" );
    PG_ASSERT( baseInfo );
    const ScriptCreateInfo* createInfo = (const ScriptCreateInfo*)baseInfo;
    name = createInfo->name;
    std::ifstream in( GetAbsPath_ScriptFilename( createInfo->filename ), std::ios::binary );
    if ( !in )
    {
        LOG_ERR( "Could not load script file '%s'", createInfo->filename.c_str() );
        return false;
    }

    in.seekg( 0, std::ios::end );
    size_t size = in.tellg();
    scriptText.resize( size );
    in.seekg( 0 );
    in.read( &scriptText[0], size ); 

    return true;
}


bool Script::FastfileLoad( Serializer* serializer )
{
    static_assert( sizeof( Script ) == 2 * sizeof( std::string ) + 8, "Dont forget to update this function when changing Script" );
    PG_ASSERT( serializer );
    serializer->Read( name );
    serializer->Read( scriptText );

    return true;
}


bool Script::FastfileSave( Serializer* serializer ) const
{
    static_assert( sizeof( Script ) == 2 * sizeof( std::string ) + 8, "Dont forget to update this function when changing Script" );
    PG_ASSERT( serializer );
    serializer->Write( name );
    serializer->Write( scriptText );

    return true;
}

} // namespace PG
