#include "asset/types/script.hpp"
#include "shared/assert.hpp"
#include "shared/logger.hpp"
#include "shared/serializer.hpp"
#include <fstream>

namespace PG
{

std::string GetAbsPath_ScriptFilename( const std::string& filename ) { return PG_ASSET_DIR + filename; }

bool Script::Load( const BaseAssetCreateInfo* baseInfo )
{
    PG_ASSERT( baseInfo );
    const ScriptCreateInfo* createInfo = (const ScriptCreateInfo*)baseInfo;
    SetName( createInfo->name );
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
    serializer->Read( scriptText );

    return true;
}

bool Script::FastfileSave( Serializer* serializer ) const
{
    PG_ASSERT( serializer );
    SerializeName( serializer );
    serializer->Write( scriptText );

    return true;
}

} // namespace PG
