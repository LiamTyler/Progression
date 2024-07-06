#include "asset/types/script.hpp"
#include "shared/assert.hpp"
#include "shared/filesystem.hpp"
#include "shared/logger.hpp"
#include "shared/serializer.hpp"

namespace PG
{

std::string GetAbsPath_ScriptFilename( const std::string& filename ) { return PG_ASSET_DIR + filename; }

bool Script::Load( const BaseAssetCreateInfo* baseInfo )
{
    const ScriptCreateInfo* createInfo = (const ScriptCreateInfo*)baseInfo;
    SetName( createInfo->name );
    FileReadResult fileData = ReadFile( GetAbsPath_ScriptFilename( createInfo->filename ) );
    if ( !fileData )
    {
        LOG_ERR( "Could not load script file '%s'", createInfo->filename.c_str() );
        return false;
    }
    scriptText.assign( fileData.data, fileData.size );

    return true;
}

bool Script::FastfileLoad( Serializer* serializer )
{
    serializer->Read( scriptText );

    return true;
}

bool Script::FastfileSave( Serializer* serializer ) const
{
    SerializeName( serializer );
    serializer->Write( scriptText );

    return true;
}

} // namespace PG
