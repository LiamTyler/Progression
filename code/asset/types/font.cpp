#include "asset/types/font.hpp"
#include "shared/assert.hpp"
#include "shared/filesystem.hpp"
#include "shared/logger.hpp"
#include "shared/serializer.hpp"

namespace PG
{

std::string GetAbsPath_FontFilename( const std::string& filename ) { return PG_ASSET_DIR + filename; }

bool Font::FastfileLoad( Serializer* serializer )
{
    DeserializeAssetName( serializer, &fontAtlasTexture );
    fontAtlasTexture.FastfileLoad( serializer );
    serializer->Read( glyphs );

    return true;
}

bool Font::FastfileSave( Serializer* serializer ) const
{
    SerializeName( serializer );
    serializer->Write( glyphs );

    return true;
}

} // namespace PG
